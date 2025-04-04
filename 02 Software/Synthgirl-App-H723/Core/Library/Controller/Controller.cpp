#include "Controller.h"

Controller::Controller()
    : lcd(LANDSCAPE_1, WHITE, BLACK, FONT_07x09),
      dac(),
      sd(),
      keyboard(),

      rhythm(),
      metronome(),
      eq(),
      osc{{0}, {1}},
      filter{{0}, {1}},
      envelope(),
      effect{{0}, {1}},
      reverb(),

      audioMetronome(0),
      audioSong(0),
      audioLpf(0),
      audioEq(0),
      audioFilter(0),
      audioEffect(0),
      audioReverb_L(0),
      audioReverb_R(0),
      audioSend_L(0),
      audioSend_R(0),

      song(),

      midiTxBusy(false),

      selectedBeatNum(-1),
      recordNoteNum(-1),
      recordBeatNum(-1),

      notePressed(false),
      notePressedBeatNum(-1),

      activeBankNum(0),
      targetBankNum(0),
      bankShiftFlag(false),
      bankActionFlag(false),

      activeOctaveNum(kInitialOctave),

      menu(INIT_MENU),
      preMenu(INIT_MENU),
      menuTab(-1),
      preMenuTab(-1),
      subMenuTab(-1),

      fileLibrarySize(0),
      synthkitLibrarySize(0),
      wavetableLibrarySize(0),

      songInterval(kMeasureInterval * kInitialMeasure * kInitialBar),
      barInterval(kMeasureInterval * kInitialMeasure),
      measureInterval(kMeasureInterval),
      playInterval(0),
      stopInterval(0),
      resetInterval(0),

      stopFlag(false),
      resetFlag(false),

      alertFlag(false),
      alertType(ALERT_OFF),

      sdInsertCheck(false),

      resetPlayFlag(false),

      fileStatus(FILE_NONE),
      synthkitStatus(FILE_NONE),

      playActive(false),
      recordActive(false),

      triggerMode(TRIG_BAR),

      playTimerPeriod(0),

      playX(0),
      playXRatio((float)songInterval / kPlayWidth),
      playColor(kPlayColor0),

      power(true),

      powerButtonFlag(false),
      powerButtonCounter(0),

      mainMenuButtonFlag(false),

      upButtonFlag(false),
      downButtonFlag(false),
      upDownButtonCounter(0),

      songBeatButtonFlag(false),
      songClearButtonFlag(false),

      songCopyButtonFlag(false),
      songPasteButtonFlag(false),

      rhythmUnlockFlag(false),
      rhythmLockFlag(false),

      textCopyFlag(false),
      textPasteFlag(false),
      textClearFlag(false),

      limitAlertShowFlag(false),
      limitAlertClearFlag(false),

      copyBeatNote(-1),
      copyBeatOctave(-1),
      copyBankSongNum(-1),

      songBeatButtonStage(0),

      fileMenuCounter(-1),
      synthkitMenuCounter(-1),

      transitionShowFlag(0),
      transitionClearFlag(false),

      animation(true),
      textShow(false) {}

Controller::~Controller() {}

void Controller::initialize() {
    lpf.initialize();
    eq.initialize();
    filter[0].initialize();
    filter[1].initialize();
    envelope.initialize();
    effect[0].initialize();
    effect[1].initialize();
    reverb.initialize();

    filter_reset(0);
    filter_reset(1);

    effect_setDData(0, EF_DELAY, kInitialDelayDry);
    effect_setEData(0, EF_DELAY, kInitialDelayWet);
    effect_setDData(1, EF_DELAY, kInitialDelayDry);
    effect_setEData(1, EF_DELAY, kInitialDelayWet);

    keyboard_initialize();
    dac_initialize();
    lcd_initialize();

    startTransitionTimer();
    stopTransitionTimer();

    startTextTimer();
    stopTextTimer();

    startPowerButtonTimer();
    stopPowerButtonTimer();

    startBeatSyncTimer();
    stopBeatSyncTimer();

    startLimitAlertTimer();
    stopLimitAlertTimer();

    LED0_ON;
    LED1_ON;
    LED2_ON;
}

void Controller::systemStart() {
    lcd.displayOn();
    dac.audioOn();

    HAL_Delay(500);

    sd_initialize();

    menu = MAIN_MENU;

    lcd_drawLogo();
    lcd_drawPage();

    updatePlayTimerPeriod();

    startSdTimer();
    startTransitionTimer();

    HAL_HalfDuplex_EnableTransmitter(&huart4);
    HAL_UART_Receive_DMA(&huart7, syncInData, 2);

    HAL_HalfDuplex_EnableReceiver(&huart1);
    HAL_HalfDuplex_EnableTransmitter(&huart6);

    HAL_UART_Receive_IT(&huart1, &midiRxData, 1);

    HAL_Delay(100);
}

void Controller::systemReset() {
    lcd.displayOff();
    HAL_Delay(1000);
    dac.audioOff();
    NVIC_SystemReset();
}

void Controller::systemUpdate_A() {
    keyboard_check_A();
}

void Controller::systemUpdate_B() {
    keyboard_check_B();
    lcd_update();

    if (!sd.ready) {
        sd_reinitialize();
    }

    if (sd.getLibrary) {
        sd.getLibrary = false;
        sd_getLibraries();
    }

    for (uint8_t i = 0; i < 2; i++) {
        if ((system.midi.txActive) && (system.midi.txTriggerNoteOn[i]) && (!midiTxBusy)) {
            if (sendMidiCommand(0x90 + system.midi.txChannel, system.midi.txDataNoteOn[i], 0x64)) {
                system.midi.txTriggerNoteOn[i] = false;
            }
        }

        if ((system.midi.txActive) && (system.midi.txTriggerNoteOff[i]) && (!midiTxBusy)) {
            if (sendMidiCommand(0x80 + system.midi.txChannel, system.midi.txDataNoteOff[i], 0x64)) {
                system.midi.txTriggerNoteOff[i] = false;
            }
        }
    }

    if (system.midi.rxNoteOnWriteFlag == 1) {
        uint8_t octaveNum = system.midi.rxNoteOnKey / 12;
        uint8_t keyNum = system.midi.rxNoteOnKey % 12;
        if ((octaveNum >= kMinOctave) && (octaveNum <= kMaxOctave)) {
            pressNote(octaveNum, keyNum);
        }
        system.midi.rxNoteOnWriteFlag = 0;
    }

    if (system.midi.rxNoteOffWriteFlag == 1) {
        if ((sD.activeBeatNum != -1) && (system.midi.rxNoteOffKey == sD.beatPlayData[sD.activeBeatNum].noteData)) {
            releaseNote();
        }
        system.midi.rxNoteOffWriteFlag = 0;
    }
}

bool Controller::sendMidiCommand(uint8_t command_, uint8_t data0_, uint8_t data1_) {
    if (!midiTxBusy) {
        uint8_t dataSend[3] = {command_, data0_, data1_};
        midiTxBusy = true;
        HAL_UART_Transmit_DMA(&huart6, dataSend, 3);
        return true;
    } else {
        return false;
    }
}

void Controller::receiveMidiCommand() {
    if (system.midi.rxActive) {
        uint8_t rxData = midiRxData;
        // note on
        if ((system.midi.rxNoteOnReadStage == 0) && (rxData == (0x90 + system.midi.rxChannel))) {
            system.midi.rxNoteOnReadStage = 1;
        } else if (system.midi.rxNoteOnReadStage == 1) {
            system.midi.rxNoteOnKey = rxData;
            system.midi.rxNoteOnReadStage = 2;
        } else if (system.midi.rxNoteOnReadStage == 2) {
            system.midi.rxNoteOnVelocity = rxData;
            system.midi.rxNoteOnReadStage = 0;
            system.midi.rxNoteOnWriteFlag = 1;
        }

        // note off
        if ((system.midi.rxNoteOffReadStage == 0) && (rxData == (0x80 + system.midi.rxChannel))) {
            system.midi.rxNoteOffReadStage = 1;
        } else if (system.midi.rxNoteOffReadStage == 1) {
            system.midi.rxNoteOffKey = rxData;
            system.midi.rxNoteOffReadStage = 2;
        } else if (system.midi.rxNoteOffReadStage == 2) {
            system.midi.rxNoteOffVelocity = rxData;
            system.midi.rxNoteOffReadStage = 0;
            system.midi.rxNoteOffWriteFlag = 1;
        }
    }
}

bool Controller::sendSyncCommand(uint8_t data_) {
    syncOutData[0] = 100;
    syncOutData[1] = data_;

    // check(syncOutData[0], 0);
    // check(syncOutData[1], 1);

    while (HAL_UART_GetState(&huart4) != HAL_UART_STATE_READY) {
    }
    if (HAL_UART_Transmit_DMA(&huart4, syncOutData, sizeof(syncOutData)) == HAL_OK) {
        return true;
    } else {
        return false;
    }
}

void Controller::receiveSyncCommand() {
    if ((system.syncIn) && (syncInData[0] == 100) && (syncInData[1])) {
        switch (syncInData[1]) {
        case SYNC_RESET:
            reset();
            break;

        case SYNC_TRIG_RESET:
            triggerReset();
            break;

        case SYNC_STOP:
            stop();
            break;

        case SYNC_TRIG_STOP:
            triggerStop();
            break;

        case SYNC_PLAY:
            play();
            break;

        case SYNC_RECORD:
            record();
            break;

        default:
            if ((syncInData[1] >= kMinTempo) && (syncInData[1] <= kMaxTempo)) {
                rhythm_setTempo(syncInData[1]);
            }
            break;
        }
    }

    // check(syncInData[0], 4);
    // check(syncInData[1], 5);

    syncInData[0] = 0x00;
    syncInData[1] = 0x00;
}

void Controller::calculateSongInterval() {
    songInterval = kMeasureInterval * rhythm.measure * rhythm.bar;
    barInterval = kMeasureInterval * rhythm.measure;
    measureInterval = kMeasureInterval;
    rhythm.measureTotal = rhythm.measure * rhythm.bar;
    metronome.precounterMax = barInterval;
}

uint16_t Controller::calculateTriggerInterval() {
    uint16_t interval;
    switch (triggerMode) {
    case TRIG_MEASURE:
        interval = ((playInterval / measureInterval + 1) * measureInterval) - 1;
        break;

    case TRIG_BAR:
        interval = ((playInterval / barInterval + 1) * barInterval) - 1;
        break;

    case TRIG_SONG:
        interval = songInterval - 1;
        break;

    default:
        break;
    }
    return interval;
}

void Controller::adjustMeasureBarTiming() {
    calculateSongInterval();
    // clear beat if its time exceeds song interval
    for (uint8_t i = 0; i < kBankLibrarySize; i++) {
        song_resetBeats(i, songInterval);
    }
}

void Controller::updatePlayTimerPeriod() {
    playTimerPeriod = 60000000 / (rhythm.tempo * kMeasureInterval);
    __HAL_TIM_SET_AUTORELOAD(&htim14, playTimerPeriod - 1);
}

/* Button functions --------------------------------------------------------*/

void Controller::button_check() {}

/* Keyboard functions --------------------------------------------------------*/

void Controller::keyboard_initialize() {
    CT0_SCL_HIGH;
    CT1_SCL_HIGH;
    CT2_SCL_HIGH;
}

void Controller::keyboard_check_A() {
    if (keyboard.leftButton >= 0) {
        // LED0_TOGGLE;
        // check(keyboard.leftButton, 0);
        switch (keyboard.leftButton) {
        case KEY_POWER:
            powerButtonFlag = true;
            powerButtonCounter = 0;
            startPowerButtonTimer();
            break;

        case LEFT_RELEASE:
            if (powerButtonFlag) {
                stopPowerButtonTimer();
                powerButtonFlag = false;
                powerButtonCounter = 0;
            }
            break;

        default:
            break;
        }
        keyboard.leftButton = -1;
    }
}

void Controller::keyboard_check_B() {
    // left button
    if (keyboard.leftButton >= 0) {
        // LED0_TOGGLE;
        // check(keyboard.leftButton, 0);
        switch (keyboard.leftButton) {
        case KEY_RESET:
            if ((!alertFlag) && (!system.sync.slaveMode)) {
                (playActive) ? triggerReset() : reset();
            }
            break;

        case KEY_PLAYSTOP:
            if ((!alertFlag) && (!system.sync.slaveMode)) {
                ((playActive) && (!stopFlag)) ? triggerStop() : play();
            }
            break;

        case KEY_REC:
            if ((!alertFlag) && (!system.sync.slaveMode)) {
                record();
            }
            break;

        case KEY_UP:
            if (!alertFlag) {
                upDownButtonCounter = 0;
                upButtonFlag = true;
                startUpDownButtonTimer();
                switch (menu) {
                case INIT_MENU:
                case MAIN_MENU:
                    break;

                case FILE_MENU:
                    file_menuUp();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuUp();
                    break;

                case SYSTEM_MENU:
                    system_menuUp();
                    break;

                case RHYTHM_MENU:
                    rhythm_menuUp();
                    break;

                case METRO_MENU:
                    metro_menuUp();
                    break;

                case EQ_MENU:
                    eq_menuUp();
                    break;

                case OSC_A0_MENU:
                case OSC_A1_MENU:
                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    osc_menuUp(osc[0]);
                    break;

                case OSC_B0_MENU:
                case OSC_B1_MENU:
                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    osc_menuUp(osc[1]);
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    filter_menuUp();
                    break;

                case ENVELOPE_MENU:
                    envelope_menuUp();
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    effect_menuUp();
                    break;

                case REVERB_MENU:
                    reverb_menuUp();
                    break;

                case KEY_MENU:
                    key_menuUp();
                    break;

                case SONG_MENU:
                    song_menuUp();
                    break;
                }
            }
            break;

        case KEY_DOWN:
            if (!alertFlag) {
                upDownButtonCounter = 0;
                downButtonFlag = true;
                startUpDownButtonTimer();
                switch (menu) {
                case INIT_MENU:
                case MAIN_MENU:
                    break;

                case FILE_MENU:
                    file_menuDown();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuDown();
                    break;

                case SYSTEM_MENU:
                    system_menuDown();
                    break;

                case RHYTHM_MENU:
                    rhythm_menuDown();
                    break;

                case METRO_MENU:
                    metro_menuDown();
                    break;

                case EQ_MENU:
                    eq_menuDown();
                    break;

                case OSC_A0_MENU:
                case OSC_A1_MENU:
                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    osc_menuDown(osc[0]);
                    break;

                case OSC_B0_MENU:
                case OSC_B1_MENU:
                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    osc_menuDown(osc[1]);
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    filter_menuDown();
                    break;

                case ENVELOPE_MENU:
                    envelope_menuDown();
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    effect_menuDown();
                    break;

                case REVERB_MENU:
                    reverb_menuDown();
                    break;

                case KEY_MENU:
                    key_menuDown();
                    break;

                case SONG_MENU:
                    song_menuDown();
                    break;
                }
            }
            break;

        case KEY_LEFT:
            if (!alertFlag) {
                switch (menu) {
                case INIT_MENU:
                case MAIN_MENU:
                    break;

                case FILE_MENU:
                    file_menuLeft();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuLeft();
                    break;

                case SYSTEM_MENU:
                    system_menuLeft();
                    break;

                case RHYTHM_MENU:
                    rhythm_menuLeft();
                    break;

                case METRO_MENU:
                    metro_menuLeft();
                    break;

                case EQ_MENU:
                    eq_menuLeft();
                    break;

                case OSC_A0_MENU:
                case OSC_A1_MENU:
                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    osc_menuLeft(osc[0]);
                    break;

                case OSC_B0_MENU:
                case OSC_B1_MENU:
                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    osc_menuLeft(osc[1]);
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    filter_menuLeft();
                    break;

                case ENVELOPE_MENU:
                    envelope_menuLeft();
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    effect_menuLeft();
                    break;

                case REVERB_MENU:
                    reverb_menuLeft();
                    break;

                case KEY_MENU:
                    key_menuLeft();
                    break;

                case SONG_MENU:
                    song_menuLeft();
                    break;
                }
            }
            break;

        case KEY_RIGHT:
            if (!alertFlag) {
                switch (menu) {
                case INIT_MENU:
                case MAIN_MENU:
                    break;

                case FILE_MENU:
                    file_menuRight();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuRight();
                    break;

                case SYSTEM_MENU:
                    system_menuRight();
                    break;

                case RHYTHM_MENU:
                    rhythm_menuRight();
                    break;

                case METRO_MENU:
                    metro_menuRight();
                    break;

                case EQ_MENU:
                    eq_menuRight();
                    break;

                case OSC_A0_MENU:
                case OSC_A1_MENU:
                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    osc_menuRight(osc[0]);
                    break;

                case OSC_B0_MENU:
                case OSC_B1_MENU:
                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    osc_menuRight(osc[1]);
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    filter_menuRight();
                    break;

                case ENVELOPE_MENU:
                    envelope_menuRight();
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    effect_menuRight();
                    break;

                case REVERB_MENU:
                    reverb_menuRight();
                    break;

                case KEY_MENU:
                    key_menuRight();
                    break;

                case SONG_MENU:
                    song_menuRight();
                    break;
                }
            }
            break;

        case KEY_CENTER:
            mainMenuButtonFlag = true;
            startLongButtonTimer();
            break;

        case KEY_ADD:
            if (alertFlag) {
                alertFlag = false;
                switch (alertType) {
                case ALERT_MEASUREUP:
                    lcd_clearAlert();
                    rhythm_setMeasure(rhythm.measure + 1);
                    break;

                case ALERT_MEASUREDOWN:
                    lcd_clearAlert();
                    rhythm_setMeasure(rhythm.measure - 1);
                    break;

                case ALERT_BARUP:
                    lcd_clearAlert();
                    rhythm_setBar(rhythm.bar + 1);
                    break;

                case ALERT_BARDOWN:
                    lcd_clearAlert();
                    rhythm_setBar(rhythm.bar - 1);
                    break;

                case ALERT_QUANTIZEUP:
                    lcd_clearAlert();
                    rhythm_setQuantize(rhythm.quantize + 1);
                    break;

                case ALERT_QUANTIZEDOWN:
                    lcd_clearAlert();
                    rhythm_setQuantize(rhythm.quantize - 1);
                    break;

                case ALERT_NEWFILE:
                    file_newAction();
                    break;

                case ALERT_LOADFILE:
                    file_loadAction();
                    break;

                case ALERT_SAVEFILE:
                case ALERT_OVERWRITEFILE:
                    file_saveAction();
                    break;

                case ALERT_CLEARFILE:
                    file_clearAction();
                    break;

                case ALERT_NEWSYNTHKIT:
                    synthkit_newAction();
                    break;

                case ALERT_LOADSYNTHKIT:
                    synthkit_loadAction();
                    break;

                case ALERT_SAVESYNTHKIT:
                case ALERT_OVERWRITESYNTHKIT:
                    synthkit_saveAction();
                    break;

                case ALERT_CLEARSYNTHKIT:
                    synthkit_clearAction();
                    break;

                default:
                    break;
                }
            } else if ((menu == SONG_MENU) && (song.bankLibrary[activeBankNum].lastActiveBeatNum == -1)) {
                songBeatButtonFlag = true;
                songBeatButtonStage = 0;
                startLongButtonTimer();
            } else if (menu == FILE_MENU) {
                switch (menuTab) {
                case 0:
                    file_newSelect();
                    break;

                case 1:
                    file_loadSelect();
                    break;

                case 2:
                    file_saveSelect();
                    break;

                case 3:
                    file_clearSelect();
                    break;
                }
            } else if (menu == SYNTHKIT_MENU) {
                switch (menuTab) {
                case 0:
                    synthkit_newSelect();
                    break;

                case 1:
                    synthkit_loadSelect();
                    break;

                case 2:
                    synthkit_saveSelect();
                    break;

                case 3:
                    synthkit_clearSelect();
                    break;
                }
            } else if (menu == RHYTHM_MENU) {
                rhythmLockFlag = true;
                startLongButtonTimer();
            } else if ((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) {
                osc_setWavetableLoaded(osc[0], true);
            } else if ((menu == OSC_B0_MENU) || (menu == OSC_B1_MENU)) {
                osc_setWavetableLoaded(osc[1], true);
            }
            break;

        case KEY_ERASE:
            if (alertFlag) {
                lcd_clearAlert();
                alertFlag = false;
            } else if ((menu == SONG_MENU) && (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)) {
                songClearButtonFlag = true;
                startLongButtonTimer();
            } else if (menu == RHYTHM_MENU) {
                rhythmUnlockFlag = true;
                startLongButtonTimer();
            }
            break;

        case KEY_COPY:
            if ((menu == SONG_MENU) && (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)) {
                noteCopyButtonFlag = true;
                songCopyButtonFlag = true;
                startLongButtonTimer();
            }
            break;

        case KEY_PASTE:
            if (menu == SONG_MENU) {
                notePasteButtonFlag = true;
                songPasteButtonFlag = true;
                startLongButtonTimer();
            }
            break;

        case KEY_POWER:
            powerButtonFlag = true;
            powerButtonCounter = 0;
            startPowerButtonTimer();
            break;

        case KEY_OCTAVE_DOWN:
            octaveDown();
            break;

        case KEY_OCTAVE_UP:
            octaveUp();
            break;

        case LEFT_RELEASE:
            if (powerButtonFlag) {
                stopPowerButtonTimer();
                powerButtonFlag = false;
                powerButtonCounter = 0;
            } else if (upButtonFlag) {
                stopUpDownButtonTimer();
                upButtonFlag = false;
                upDownButtonCounter = 0;
            } else if (downButtonFlag) {
                stopUpDownButtonTimer();
                downButtonFlag = false;
                upDownButtonCounter = 0;
            } else if (songBeatButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                songBeatButtonFlag = false;
                songBeatButtonStage = 0;
            } else if (songClearButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                songClearButtonFlag = false;
                song_resetSelectedBeat();
            } else if (noteCopyButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                if (songCopyButtonFlag) {
                    noteCopyButtonFlag = false;
                    songCopyButtonFlag = false;
                    if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) {
                        copyBeatNote = song.bankLibrary[activeBankNum].beatLibrary[selectedBeatNum].note;
                        copyBeatOctave = song.bankLibrary[activeBankNum].beatLibrary[selectedBeatNum].octave;
                    }
                } else {
                    noteCopyButtonFlag = false;
                }
            } else if (notePasteButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                if (songPasteButtonFlag) {
                    if ((copyBeatNote != -1) && (copyBeatOctave != -1) && (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)) {
                        song_setBeatNoteOctave(copyBeatNote, copyBeatOctave);
                    }
                    notePasteButtonFlag = false;
                    songPasteButtonFlag = false;
                } else {
                    notePasteButtonFlag = false;
                }
            } else if (mainMenuButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                mainMenuButtonFlag = false;
                if (menu == EQ_MENU) {
                    switch (subMenuTab) {
                    case 0:
                        subMenuTab = 1;
                        break;

                    case 1:
                        if ((menuTab == 2) || (menuTab == 7)) {
                            subMenuTab = 0;
                        } else {
                            subMenuTab = 2;
                        }
                        break;

                    case 2:
                        subMenuTab = 0;
                        break;

                    default:
                        break;
                    }
                    lcd_transitionSelect();
                }
            } else if (rhythmUnlockFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                rhythmUnlockFlag = false;
            } else if (rhythmLockFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                rhythmLockFlag = false;
            }
            break;

        default:
            break;
        }
        keyboard.leftButton = -1;
    }

    // right button
    if (keyboard.rightButton >= 0) {
        // LED1_TOGGLE;
        // check(keyboard.rightButton, 0);
        switch (keyboard.rightButton) {
        case KEY_FILE:
            if (!alertFlag)
                file_select();
            break;

        case KEY_SYNTHKIT:
            if (!alertFlag)
                synthkit_select();
            break;

        case KEY_SYSTEM:
            if (!alertFlag)
                system_select();
            break;

        case KEY_RHYTHM:
            if (!alertFlag)
                rhythm_select();
            break;

        case KEY_METRONOME:
            if (!alertFlag)
                metro_select();
            break;

        case KEY_EQ:
            if (!alertFlag)
                eq_select();
            break;

        case KEY_OSC_A:
            if (!alertFlag)
                osc_select(osc[0]);
            break;

        case KEY_OSC_B:
            if (!alertFlag)
                osc_select(osc[1]);
            break;

        case KEY_FILTER:
            if (!alertFlag)
                filter_select();
            break;

        case KEY_ENVELOPE:
            if (!alertFlag)
                envelope_select();
            break;

        case KEY_EFFECT:
            if (!alertFlag)
                effect_select();
            break;

        case KEY_REVERB:
            if (!alertFlag)
                reverb_select();
            break;

        case KEY_MODE_KEY:
            if (!alertFlag)
                key_select();
            break;

        case KEY_MODE_SONG:
            if (!alertFlag)
                song_select();
            break;

        case KEY_BANK:
            keyboard.bankButtonPress = true;
            break;

        case RIGHT_RELEASE:
            keyboard.bankButtonPress = false;
            break;

        default:
            break;
        }
        keyboard.rightButton = -1;
    }

    // beat button
    if (keyboard.beatButton >= 0) {
        // LED2_TOGGLE;
        // check(keyboard.beatButton, 0);
        switch (keyboard.beatButton) {
        case KEY_00:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(0) : bank_select(0);
            } else {
                pressNote(activeOctaveNum, 0);
            }
            break;

        case KEY_01:
            pressNote(activeOctaveNum, 1);
            break;

        case KEY_02:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(1) : bank_select(1);
            } else {
                pressNote(activeOctaveNum, 2);
            }
            break;

        case KEY_03:
            pressNote(activeOctaveNum, 3);
            break;

        case KEY_04:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(2) : bank_select(2);
            } else {
                pressNote(activeOctaveNum, 4);
            }
            break;

        case KEY_05:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(3) : bank_select(3);
            } else {
                pressNote(activeOctaveNum, 5);
            }
            break;

        case KEY_06:
            pressNote(activeOctaveNum, 6);
            break;

        case KEY_07:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(4) : bank_select(4);
            } else {
                pressNote(activeOctaveNum, 7);
            }
            break;

        case KEY_08:
            pressNote(activeOctaveNum, 8);
            break;

        case KEY_09:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(5) : bank_select(5);
            } else {
                pressNote(activeOctaveNum, 9);
            }
            break;

        case KEY_10:
            pressNote(activeOctaveNum, 10);
            break;

        case KEY_11:
            if (keyboard.bankButtonPress) {
                (playActive) ? bank_trigger(6) : bank_select(6);
            } else {
                pressNote(activeOctaveNum, 11);
            }
            break;

        case BEAT_RELEASE:
            if (keyboard.bankButtonPress) {
            } else {
                releaseNote();
            }
            break;

        default:
            break;
        }
        keyboard.beatButton = -1;
    }
}

void Controller::keyboard_check_C() {
    if (keyboard.leftButton >= 0) {
        // LED0_TOGGLE;
        // check(keyboard.leftButton, 0);
        switch (keyboard.leftButton) {
        case KEY_POWER:
            powerButtonFlag = true;
            powerButtonCounter = 0;
            startPowerButtonTimer();
            break;

        case LEFT_RELEASE:
            if (powerButtonFlag) {
                stopPowerButtonTimer();
                powerButtonFlag = false;
                powerButtonCounter = 0;
            }
            break;

        default:
            break;
        }
        keyboard.leftButton = -1;
    }
}

void Controller::keyboard_enable() {
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void Controller::keyboard_disable() {
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    HAL_NVIC_DisableIRQ(EXTI3_IRQn);
    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
}

/* Dac functions -------------------------------------------------------------*/

void Controller::dac_initialize() { dac.initialize(); }

/* Sd functions --------------------------------------------------------------*/

SdResult Controller::sd_initialize() {
    SdResult sdResult = SD_ERROR;

    while (sdResult != SD_OK) {
        if (sd_detect() == SD_OK) {
            sd.detect = true;
            if (sdInsertCheck) {
                lcd.drawInitSdReadAlert();
                sdInsertCheck = false;
            }
            FATFS_UnLinkDriver(SDPath);
            FATFS_LinkDriver(&SD_Driver, SDPath);
            if ((sd_mount() == SD_OK) && (sd_getLabel() == SD_OK) &&
                (sd_getSpace() == SD_OK)) {
                sd.serialTemp = sd.serial;
                if (sd_checkFolderExist("System") == SD_OK) {
                    if (sd_checkFolderExist("Wavetable") == SD_OK) {
                        if (sd_checkFolderExist("System/File") == SD_OK) {
                            if (sd_checkFolderExist("System/Synthkit") == SD_OK) {
                                if (sd_checkFolderExist("System/Sound") == SD_OK) {
                                    if (sd_checkFolderExist("System/Image") == SD_OK) {
                                        if (sd_checkFolderExist("System/Firmware") == SD_OK) {
                                            if ((sd_checkMetronome() == SD_OK) && (sd_loadMetronome() == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Logo.rwi", RAM_IMAGE_LOGO_PALETTE_ADDRESS, RAM_IMAGE_LOGO_DATA_ADDRESS, kImageLogoPalette, kImageLogoWidth, kImageLogoHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Menu.rwi", RAM_IMAGE_MENU_PALETTE_ADDRESS, RAM_IMAGE_MENU_DATA_ADDRESS, kImageMenuPalette, kImageMenuWidth, kImageMenuHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Key.rwi", RAM_IMAGE_KEY_PALETTE_ADDRESS, RAM_IMAGE_KEY_DATA_ADDRESS, kImageKeyPalette, kImageKeyWidth, kImageKeyHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Osc_A.rwi", RAM_IMAGE_OSC_A_PALETTE_ADDRESS, RAM_IMAGE_OSC_A_DATA_ADDRESS, kImageOscAPalette, kImageOscAWidth, kImageOscAHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Osc_B.rwi", RAM_IMAGE_OSC_B_PALETTE_ADDRESS, RAM_IMAGE_OSC_B_DATA_ADDRESS, kImageOscBPalette, kImageOscBWidth, kImageOscBHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Filter.rwi", RAM_IMAGE_FILTER_PALETTE_ADDRESS, RAM_IMAGE_FILTER_DATA_ADDRESS, kImageFilterPalette, kImageFilterWidth, kImageFilterHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Image_Envelope.rwi", RAM_IMAGE_ENV_PALETTE_ADDRESS, RAM_IMAGE_ENV_DATA_ADDRESS, kImageEnvPalette, kImageEnvWidth, kImageEnvHeight, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Icon_Select.rwi", RAM_ICON_SELECT_PALETTE_ADDRESS, RAM_ICON_SELECT_DATA_ADDRESS, kIconPalette, kIconSelectWidth, kIconSelectHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Icon_Alert.rwi", RAM_ICON_ALERT_PALETTE_ADDRESS, RAM_ICON_ALERT_DATA_ADDRESS, kIconPalette, kIconAlertWidth, kIconAlertHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Icon_Play.rwi", RAM_ICON_PLAY_PALETTE_ADDRESS, RAM_ICON_PLAY_DATA_ADDRESS, kIconPalette, kIconPlayWidth, kIconPlayHeight * 8, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Key.rwi", RAM_GRAPH_KEY_PALETTE_ADDRESS, RAM_GRAPH_KEY_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 13, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Arpeg.rwi", RAM_GRAPH_ARPEG_PALETTE_ADDRESS, RAM_GRAPH_ARPEG_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 10, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Lfo_A.rwi", RAM_GRAPH_LFO_A_PALETTE_ADDRESS, RAM_GRAPH_LFO_A_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 26, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Lfo_B.rwi", RAM_GRAPH_LFO_B_PALETTE_ADDRESS, RAM_GRAPH_LFO_B_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 26, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Envelope.rwi", RAM_GRAPH_ENVELOPE_PALETTE_ADDRESS, RAM_GRAPH_ENVELOPE_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 4, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Graph_Filter.rwi", RAM_GRAPH_FILTER_PALETTE_ADDRESS, RAM_GRAPH_FILTER_DATA_ADDRESS, kInfoGraphPalette, kInfoGraphWidth, kInfoGraphHeight * 5, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Key.rwi", RAM_BUTTON_KEY_PALETTE_ADDRESS, RAM_BUTTON_KEY_DATA_ADDRESS, kButtonKeyPalette, kButtonKeyWidth, kButtonKeyHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Osc_A.rwi", RAM_BUTTON_OSC_A_PALETTE_ADDRESS, RAM_BUTTON_OSC_A_DATA_ADDRESS, kButtonOscAPalette, kButtonOscAWidth, kButtonOscAHeight * 3, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Osc_B.rwi", RAM_BUTTON_OSC_B_PALETTE_ADDRESS, RAM_BUTTON_OSC_B_DATA_ADDRESS, kButtonOscBPalette, kButtonOscBWidth, kButtonOscBHeight * 3, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Filter.rwi", RAM_BUTTON_FILTER_PALETTE_ADDRESS, RAM_BUTTON_FILTER_DATA_ADDRESS, kButtonFilterPalette, kButtonFilterWidth, kButtonFilterHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Envelope.rwi", RAM_BUTTON_ENVELOPE_PALETTE_ADDRESS, RAM_BUTTON_ENVELOPE_DATA_ADDRESS, kButtonEnvelopePalette, kButtonEnvelopeWidth, kButtonEnvelopeHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Osc_A_Lfo.rwi", RAM_BUTTON_OSC_A_LFO_PALETTE_ADDRESS, RAM_BUTTON_OSC_A_LFO_DATA_ADDRESS, kButtonLfoPalette, kButtonLfoWidth, kButtonLfoHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Osc_B_Lfo.rwi", RAM_BUTTON_OSC_B_LFO_PALETTE_ADDRESS, RAM_BUTTON_OSC_B_LFO_DATA_ADDRESS, kButtonLfoPalette, kButtonLfoWidth, kButtonLfoHeight * 2, RGB16) == SD_OK) &&
                                                (sd_loadImage("System/Image/Button_Song.rwi", RAM_BUTTON_SONG_PALETTE_ADDRESS, RAM_BUTTON_SONG_DATA_ADDRESS, kButtonSongPalette, kButtonSongWidth, kButtonSongHeight * 2, RGB16) == SD_OK)) {
                                                sd.ready = true;
                                                sd.getLibrary = true;
                                                sdResult = SD_OK;
                                            } else {
                                                sdResult = SD_ERROR_SYSTEMFILE;
                                            }
                                        } else {
                                            sdResult = SD_ERROR_FIRMWAREFOLDER;
                                        }
                                    } else {
                                        sdResult = SD_ERROR_IMAGEFOLDER;
                                    }
                                } else {
                                    sdResult = SD_ERROR_SOUNDFOLDER;
                                }
                            } else {
                                sdResult = SD_ERROR_SYNTHKITFOLDER;
                            }
                        } else {
                            sdResult = SD_ERROR_FILEFOLDER;
                        }
                    } else {
                        sdResult = SD_ERROR_WAVETABLEFOLDER;
                    }
                } else {
                    sdResult = SD_ERROR_SYSTEMFOLDER;
                }
            } else {
                sdResult = SD_ERROR_MOUNT;
            }
        } else {
            sd.detect = false;
            sdResult = SD_ERROR_DETECT;
        }

        if (sdResult != SD_OK) {
            lcd_drawInitSdAlert(sdResult);
            sdInsertCheck = true;
            if (sdResult == SD_ERROR_DETECT) {
                while (sd_detect() != SD_OK) {
                    keyboard_check_C();
                    if (!power)
                        systemReset();
                }
            } else {
                sd_unmount();
                while (sd_detect() == SD_OK) {
                    keyboard_check_C();
                    if (!power)
                        systemReset();
                }
            }
            HAL_Delay(500);
        }
    }
    lcd_clearInitSdAlert();

    return sdResult;
}

SdResult Controller::sd_reinitialize() {
    SdResult sdResult = SD_ERROR;

    while (sdResult != SD_OK) {
        if (sd_detect() == SD_OK) {
            sd.detect = true;
            FATFS_UnLinkDriver(SDPath);
            FATFS_LinkDriver(&SD_Driver, SDPath);
            if ((sd_mount() == SD_OK) && (sd_getLabel() == SD_OK) && (sd_getSpace() == SD_OK)) {
                if (sd.serial == sd.serialTemp) {
                    if (sd_checkFolderExist("System") == SD_OK) {
                        if (sd_checkFolderExist("Wavetable") == SD_OK) {
                            if (sd_checkFolderExist("System/File") == SD_OK) {
                                if (sd_checkFolderExist("System/Synthkit") == SD_OK) {
                                    if (sd_checkFolderExist("System/Sound") == SD_OK) {
                                        if (sd_checkFolderExist("System/Image") == SD_OK) {
                                            if (sd_checkFolderExist("System/Firmware") == SD_OK) {
                                                sd.ready = true;
                                                sd.getLibrary = true;
                                                sdResult = SD_OK;
                                            } else {
                                                sdResult = SD_ERROR_FIRMWAREFOLDER;
                                            }
                                        } else {
                                            sdResult = SD_ERROR_IMAGEFOLDER;
                                        }
                                    } else {
                                        sdResult = SD_ERROR_SOUNDFOLDER;
                                    }
                                } else {
                                    sdResult = SD_ERROR_SYNTHKITFOLDER;
                                }
                            } else {
                                sdResult = SD_ERROR_FILEFOLDER;
                            }
                        } else {
                            sdResult = SD_ERROR_WAVETABLEFOLDER;
                        }
                    } else {
                        sdResult = SD_ERROR_SYSTEMFOLDER;
                    }
                } else {
                    sdResult = SD_ERROR_SERIAL;
                }
            } else {
                sdResult = SD_ERROR_MOUNT;
            }
        } else {
            sd.detect = false;
            sdResult = SD_ERROR_DETECT;
        }

        if (sdResult != SD_OK) {
            lcd_drawSdAlert(sdResult);
            if (sdResult == SD_ERROR_DETECT) {
                while (sd_detect() != SD_OK) {
                    lcd_update();
                    keyboard_check_C();
                    if (!power)
                        systemReset();
                }
            } else {
                sd_unmount();
                while (sd_detect() == SD_OK) {
                    lcd_update();
                    keyboard_check_C();
                    if (!power)
                        systemReset();
                }
            }
            HAL_Delay(500);
        }
    }
    lcd_clearSdAlert();
    // clear pre-alert
    alertFlag = false;
    alertType = ALERT_OFF;
    // go to main menu
    main_select();

    return sdResult;
}

SdResult Controller::sd_detect() {
    SdResult sdResult;
    (HAL_GPIO_ReadPin(SDMMC2_DETECT_GPIO_Port, SDMMC2_DETECT_Pin)) ? sdResult = SD_ERROR : sdResult = SD_OK;
    return sdResult;
}

SdResult Controller::sd_mount() {
    SdResult sdResult;
    (f_mount(&sd.fs, SDPath, 0) == FR_OK) ? sdResult = SD_OK : sdResult = SD_ERROR;
    return sdResult;
}

SdResult Controller::sd_unmount() {
    SdResult sdResult;
    (f_mount(0, SDPath, 0) == FR_OK) ? sdResult = SD_OK : sdResult = SD_ERROR;
    return sdResult;
}

SdResult Controller::sd_getLabel() {
    SdResult sdResult;
    (f_getlabel(SDPath, sd.label, &sd.serial) == FR_OK) ? sdResult = SD_OK : sdResult = SD_ERROR;
    return sdResult;
}

SdResult Controller::sd_setLabel() {
    SdResult sdResult;
    (f_setlabel("SYNTHKIT")) ? sdResult = SD_OK : sdResult = SD_ERROR;
    return sdResult;
}

SdResult Controller::sd_getSpace() {
    SdResult sdResult;
    FATFS *fs_ptr = &sd.fs;
    uint32_t freeCluster;
    if (f_getfree(SDPath, (DWORD *)&freeCluster, &fs_ptr) == FR_OK) {
        uint32_t totalBlocks = (sd.fs.n_fatent - 2) * sd.fs.csize;
        uint32_t freeBlocks = freeCluster * sd.fs.csize;
        sd.totalSpace = totalBlocks / 2000;
        sd.freeSpace = freeBlocks / 2000;
        sd.usedSpace = sd.totalSpace - sd.freeSpace;
        sdResult = SD_OK;
    } else {
        sd.totalSpace = 0;
        sd.freeSpace = 0;
        sd.usedSpace = 0;
        sdResult = SD_ERROR;
    }
    return sdResult;
}

SdResult Controller::sd_checkFileExist(char *fileAddress) {
    SdResult sdResult;
    if ((f_stat(fileAddress, &sd.fileInfo) == FR_OK) && ((sd.fileInfo.fattrib & AM_DIR) == false)) {
        sdResult = SD_OK;
    } else {
        sdResult = SD_ERROR;
    }
    return sdResult;
}

SdResult Controller::sd_checkFolderExist(char *folderAddress) {
    SdResult sdResult;
    if ((f_stat(folderAddress, &sd.fileInfo) == FR_OK) && (sd.fileInfo.fattrib & AM_DIR)) {
        sdResult = SD_OK;
    } else {
        sdResult = SD_ERROR;
    }
    return sdResult;
}

SdResult Controller::sd_loadImage(char *fileAddress, uint32_t paletteAddress, uint32_t dataAddress, uint16_t paletteSize, uint16_t width, uint16_t height, RGBMode mode) {
    SdResult sdResult = SD_ERROR;
    char data[20] = "";
    char headerTitle[] = "RW_IMAGE  ";
    char headerIndex[] = "_PA";
    char headerData[] = "_DA";
    uint32_t errorCount = 0;

    uint16_t rgb;
    (mode) ? rgb = 24 : rgb = 16;

    // disable keyboard
    keyboard_disable();

    if (f_open(&sd.file, fileAddress, FA_READ) == FR_OK) {
        if (f_read(&sd.file, data, 10, &sd.bytesread) == FR_OK) {
            if (strcmp(data, headerTitle) == 0) {
                memset(data, 0x00, sizeof(data));
                if (f_read(&sd.file, data, 8, &sd.bytesread) == FR_OK) {
                    uint16_t *paletteSize_ = (uint16_t *)&(data[0]);
                    uint16_t *rgb_ = (uint16_t *)&(data[2]);
                    uint16_t *width_ = (uint16_t *)&(data[4]);
                    uint16_t *height_ = (uint16_t *)&(data[6]);
                    if ((paletteSize == *paletteSize_) && (rgb == *rgb_) && (width == *width_) && (height == *height_)) {
                        memset(data, 0, sizeof(data));
                        f_read(&sd.file, data, 3, &sd.bytesread);
                        if (strcmp(data, headerIndex) == 0) {
                            memset(data, 0, sizeof(data));
                            if (mode == RGB16) {
                                const uint16_t readByteSize = 512;
                                uint16_t paletteByteSize = paletteSize * 2;
                                uint16_t readCountTotal = (paletteByteSize / readByteSize);
                                uint16_t remainderByteSize = (paletteByteSize % readByteSize);
                                uint8_t dataRead[readByteSize] = {0};
                                uint32_t pointerOffset = 0;

                                for (uint16_t i = 0; i < readCountTotal; i++) {
                                    if (f_read(&sd.file, &dataRead, readByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t j = 0; j < readByteSize; j++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(paletteAddress + pointerOffset);
                                        *writePtr = dataRead[j];
                                        pointerOffset += 1;
                                    }
                                    memset(dataRead, 0x00, readByteSize);
                                }
                                if (remainderByteSize) {
                                    if (f_read(&sd.file, &dataRead, remainderByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(paletteAddress + pointerOffset);
                                        *writePtr = dataRead[k];
                                        pointerOffset += 1;
                                    }
                                }
                            } else if (mode == RGB24) {
                                const uint16_t readByteSize = 512;
                                uint16_t paletteByteSize = paletteSize * 3;
                                uint16_t readCountTotal = (paletteByteSize / readByteSize);
                                uint16_t remainderByteSize = (paletteByteSize % readByteSize);
                                uint8_t dataRead[readByteSize] = {0};
                                uint32_t pointerOffset = 0;

                                for (uint16_t i = 0; i < readCountTotal; i++) {
                                    if (f_read(&sd.file, &dataRead, readByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t j = 0; j < readByteSize; j++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(paletteAddress + pointerOffset);
                                        *writePtr = dataRead[j];
                                        pointerOffset += 1;
                                    }
                                    memset(dataRead, 0x00, readByteSize);
                                }
                                if (remainderByteSize) {
                                    if (f_read(&sd.file, &dataRead, remainderByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(paletteAddress + pointerOffset);
                                        *writePtr = dataRead[k];
                                        pointerOffset += 1;
                                    }
                                }
                            }

                            f_read(&sd.file, data, 3, &sd.bytesread);
                            if (strcmp(data, headerData) == 0) {
                                memset(data, 0, sizeof(data));
                                const uint16_t readByteSize = 512;
                                uint32_t pixelByteSize = width * height;
                                uint16_t readCountTotal = (pixelByteSize / readByteSize);
                                uint16_t remainderByteSize = pixelByteSize % readByteSize;
                                uint8_t dataRead[readByteSize] = {0};
                                uint32_t pointerOffset = 0;
                                uint32_t errorCount = 0;

                                for (uint32_t i = 0; i < readCountTotal; i++) {
                                    if (f_read(&sd.file, &dataRead, readByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t j = 0; j < readByteSize; j++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(dataAddress + pointerOffset);
                                        *writePtr = dataRead[j];
                                        pointerOffset += 1;
                                    }
                                }
                                if (remainderByteSize) {
                                    if (f_read(&sd.file, &dataRead, remainderByteSize, &sd.bytesread) != FR_OK)
                                        errorCount += 1;
                                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                                        volatile uint8_t *writePtr = (volatile uint8_t *)(dataAddress + pointerOffset);
                                        *writePtr = dataRead[k];
                                        pointerOffset += 1;
                                    }
                                }
                                if (errorCount == 0)
                                    sdResult = SD_OK;
                            }
                        }
                    }
                }
            }
        }
    }
    f_close(&sd.file);
    // enable keyboard
    keyboard_enable();
    return sdResult;
}

SdResult Controller::load16BitAudio(char *fileAddress, uint32_t ramAddress, uint32_t sampleSize) {
    SdResult sdResult = SD_ERROR;
    struct WavData wavData;

    // disable keyboard
    keyboard_disable();
    // read wav file
    if (f_open(&sd.file, fileAddress, FA_READ) == FR_OK) {
        if (f_read(&sd.file, &wavData.riff_chunk, 12, &sd.bytesread) == FR_OK) {
            // read riff_chunk
            if ((wavData.riff_chunk.chunkId == 0x46464952) && (wavData.riff_chunk.fileFormat == 0x45564157)) {
                uint32_t chunkSize = wavData.riff_chunk.chunkSize + 8;
                // read fmt_chunk
                for (uint32_t i = 0; i < (chunkSize - 24); i++) {
                    f_lseek(&sd.file, i);
                    f_read(&sd.file, &wavData.fmt_chunk, 24, &sd.bytesread);
                    if (wavData.fmt_chunk.chunkId == 0x20746D66) {
                        wavData.fmt_chunk.chunkStartByte = i;
                        break;
                    }
                }
                // check fmt_chunk
                if ((wavData.fmt_chunk.chunkId == 0x20746D66) && (wavData.fmt_chunk.chunkSize == 16) &&
                    (wavData.fmt_chunk.audioFormat == 0x01) && (wavData.fmt_chunk.nbrChannels == 0x01) &&
                    (wavData.fmt_chunk.sampleRate == 48000) && (wavData.fmt_chunk.bitPerSample == 16)) {
                    // read data_chunk
                    for (uint32_t j = 12; j < (chunkSize - 8); j++) {
                        f_lseek(&sd.file, j);
                        f_read(&sd.file, &wavData.data_chunk, 8, &sd.bytesread);
                        if ((wavData.data_chunk.chunkId == 0x61746164)) {
                            wavData.data_chunk.chunkStartByte = j;
                            break;
                        }
                    }
                    // check data chunk
                    uint32_t byteSize = sampleSize * 2;
                    if ((wavData.data_chunk.chunkId == 0x61746164) && (wavData.data_chunk.chunkSize == byteSize)) {
                        if (f_read(&sd.file, (uint8_t *)ramAddress, byteSize, &sd.bytesread) == FR_OK) {
                            sdResult = SD_OK;
                        }
                    }
                }
            }
        }
    }
    f_close(&sd.file);
    // enable keyboard
    keyboard_enable();
    return sdResult;
}

SdResult Controller::load24BitAudio(char *fileAddress, uint32_t ramAddress, uint32_t sampleSize) {
    SdResult sdResult = SD_ERROR;
    struct WavData wavData;

    // disable keyboard
    keyboard_disable();
    // read wav file
    if (f_open(&sd.file, fileAddress, FA_READ) == FR_OK) {
        if (f_read(&sd.file, &wavData.riff_chunk, 12, &sd.bytesread) == FR_OK) {
            // read riff_chunk
            if ((wavData.riff_chunk.chunkId == 0x46464952) && (wavData.riff_chunk.fileFormat == 0x45564157)) {
                uint32_t chunkSize = wavData.riff_chunk.chunkSize + 8;
                // read fmt_chunk
                for (uint32_t i = 0; i < (chunkSize - 24); i++) {
                    f_lseek(&sd.file, i);
                    f_read(&sd.file, &wavData.fmt_chunk, 24, &sd.bytesread);
                    if (wavData.fmt_chunk.chunkId == 0x20746D66) {
                        wavData.fmt_chunk.chunkStartByte = i;
                        break;
                    }
                }
                // check fmt_chunk
                if ((wavData.fmt_chunk.chunkId == 0x20746D66) && (wavData.fmt_chunk.chunkSize == 16) &&
                    (wavData.fmt_chunk.audioFormat == 0x01) && (wavData.fmt_chunk.nbrChannels == 0x01) &&
                    (wavData.fmt_chunk.sampleRate == 48000) && (wavData.fmt_chunk.bitPerSample == 24)) {
                    // read data_chunk
                    for (uint32_t j = 12; j < (chunkSize - 8); j++) {
                        f_lseek(&sd.file, j);
                        f_read(&sd.file, &wavData.data_chunk, 8, &sd.bytesread);
                        if ((wavData.data_chunk.chunkId == 0x61746164)) {
                            wavData.data_chunk.chunkStartByte = j;
                            break;
                        }
                    }
                    // check data chunk
                    uint32_t byteSize = sampleSize * 3;
                    if ((wavData.data_chunk.chunkId == 0x61746164) && (wavData.data_chunk.chunkSize == byteSize)) {
                        if (f_read(&sd.file, (uint8_t *)ramAddress, byteSize, &sd.bytesread) == FR_OK) {
                            sdResult = SD_OK;
                        }
                    }
                }
            }
        }
    }
    f_close(&sd.file);
    // enable keyboard
    keyboard_enable();
    return sdResult;
}

SdResult Controller::sd_checkMetronome() {
    return sd_checkFileExist("System/Sound/Metronome.wav");
}

SdResult Controller::sd_loadMetronome() {
    SdResult sdResult = load24BitAudio("System/Sound/Metronome.wav", RAM_METRO_ADDRESS, kMetroSize * 10);
    return sdResult;
}

void Controller::sd_getLibraries() {
    lcd_clearSdData();
    lcd_drawSdDataIntro();

    sd_deleteDirectory("System/Data");

    sd_getFileLibrary();
    sd_getSynthkitLibrary();
    sd_getWavetableLibrary();
    sd_checkWavetablesInUse();

    lcd_drawSdData();
}

SdResult Controller::sd_getFileLibrary() {
    SdResult result = SD_ERROR;
    char refRead[sizeof(kFileRef)] = {};
    char data;

    // disable keyboard
    keyboard_disable();
    // read file library
    fileLibrarySize = 0;
    for (uint8_t i = 0; i < kFileLibrarySize; i++) {
        char fileName[50];
        char fileStart[] = "System/File/File_";
        char fileEnd[] = ".rws";
        char fileNum[3];

        sprintf(fileNum, "%03d", (i + 1));
        strcpy(fileName, fileStart);
        strcat(fileName, fileNum);
        strcat(fileName, fileEnd);

        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            if (f_read(&sd.file, refRead, strlen(kFileRef), &sd.bytesread) == 0) {
                if (f_read(&sd.file, &data, 1, &sd.bytesread) == FR_OK) {
                    if (data == '*')
                        fileLibrarySize += 1;
                }
            }
        }
        f_close(&sd.file);
    }

    result = SD_OK;
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_getSynthkitLibrary() {
    SdResult result = SD_ERROR;
    char refRead[sizeof(kSynthkitRef)] = {};
    char data;

    // disable keyboard
    keyboard_disable();
    // read synthkit library
    synthkitLibrarySize = 0;
    for (uint8_t i = 0; i < kSynthkitLibrarySize; i++) {
        char fileName[50];
        char fileStart[] = "System/Synthkit/Synthkit_";
        char fileEnd[] = ".rws";
        char fileNum[3];

        sprintf(fileNum, "%03d", (i + 1));
        strcpy(fileName, fileStart);
        strcat(fileName, fileNum);
        strcat(fileName, fileEnd);

        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            if (f_read(&sd.file, refRead, strlen(kSynthkitRef), &sd.bytesread) == 0) {
                if (f_read(&sd.file, &data, 1, &sd.bytesread) == FR_OK) {
                    if (data == '*')
                        synthkitLibrarySize += 1;
                }
            }
        }
        f_close(&sd.file);
    }

    result = SD_OK;
    // enable keyboard
    keyboard_enable();
    return result;
}

void Controller::sd_getWavetableLibrary() {
    bool listFile;
    wavetableLibrarySize = 0;

    // disable keyboard
    keyboard_disable();
    // read wavetable library
    if (f_opendir(&sd.dir, "/Wavetable\0") == FR_OK) {
        sd.fresult = f_findfirst(&sd.dir, &sd.fileInfo, "/Wavetable", "?*.WAV");
        if (sd.fileInfo.fname[0] == '\0') {
            listFile = false;
            wavetableLibrarySize = 0;
        } else {
            listFile = true;
            while ((sd.fresult == FR_OK) && (sd.fileInfo.fname[0]) && (wavetableLibrarySize <= kWavetableLibraryMaxSize)) {
                if (!(sd.fileInfo.fattrib & AM_HID) && (!(sd.fileInfo.fattrib & AM_DIR)) && (!(sd.fileInfo.fname[0] == '.'))) {
                    char *ptr = (char *)(RAM_WAVETABLE_ADDRESS + (wavetableLibrarySize * kFileNameSize));
                    memset(ptr, 0x00, kFileNameSize);
                    strncpy(ptr, sd.fileInfo.fname, kFileNameSize);
                    wavetableLibrarySize += 1;
                }
                f_findnext(&sd.dir, &sd.fileInfo);
            }
        }
    }
    f_closedir(&sd.dir);

    char *nameArray[wavetableLibrarySize];
    for (uint16_t i = 0; i < wavetableLibrarySize; i++) {
        nameArray[i] = (char *)(RAM_WAVETABLE_ADDRESS + (i * kFileNameSize));
    }
    sortWords(nameArray, wavetableLibrarySize);

    if ((f_mkdir("System/Data") == FR_OK) && (f_open(&sd.file, "System/Data/Wavetable.lib", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)) {
        f_write(&sd.file, "RW_SYNTHGIRL_WAVETABLE   ", 25, &sd.byteswritten);
        for (uint16_t i = 0; i < wavetableLibrarySize; i++) {
            f_write(&sd.file, nameArray[i], kFileNameSize, &sd.byteswritten);
        }
        f_write(&sd.file, "EOF", 3, &sd.byteswritten);
    }
    f_close(&sd.file);
    // enable keyboard
    keyboard_enable();
}

SdResult Controller::sd_checkFile(uint8_t fileNum_) {
    SdResult result = SD_ERROR;
    fileStatus = FILE_NONE;
    char refRead[sizeof(kFileRef)] = {};
    char data;

    char fileName[50];
    char fileNum[4];
    sprintf(fileNum, "%03d", (fileNum_ + 1));
    strcpy(fileName, kFileStart);
    strcat(fileName, fileNum);
    strcat(fileName, kFileEnd);

    if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
        if (f_read(&sd.file, refRead, strlen(kFileRef), &sd.bytesread) == FR_OK) {
            if (strncmp(kFileRef, refRead, strlen(kFileRef)) == 0) {
                f_read(&sd.file, &data, 1, &sd.bytesread);
                (data == '*') ? fileStatus = FILE_ACTIVE : fileStatus = FILE_INACTIVE;
            } else {
                fileStatus = FILE_INCOMPATIBLE;
            }
        } else {
            fileStatus = FILE_INCOMPATIBLE;
        }
    } else {
        fileStatus = FILE_MISSING;
    }
    f_close(&sd.file);
    result = SD_OK;
    return result;
}

SdResult Controller::sd_loadFile(uint8_t fileNum_) {
    SdResult result = SD_ERROR;

    char fileName[50];
    char fileNum[4];
    sprintf(fileNum, "%03d", (fileNum_ + 1));
    strcpy(fileName, kFileStart);
    strcat(fileName, fileNum);
    strcat(fileName, kFileEnd);

    // 000.000 *              0001 byte
    // 000.001 main           0196 bytes
    // 000.200 wavetable      0200 bytes
    // 000.300 song           0100 bytes
    // total size             4788 bytes

    // disable keyboard
    keyboard_disable();
    // load file
    if ((sd_checkFile(fileNum_) == SD_OK) && (fileStatus == FILE_ACTIVE)) {
        char data[kFileByteSize];
        if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
            if (f_lseek(&sd.file, strlen(kFileRef)) == FR_OK) {
                const uint16_t readByteSize = 512;
                const uint16_t fileByteSize = kFileByteSize;
                uint16_t readCountTotal = (fileByteSize / readByteSize);
                uint16_t remainderByteSize = (fileByteSize % readByteSize);
                uint8_t dataRead[readByteSize] = {0};
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < readCountTotal; i++) {
                    if (f_read(&sd.file, &dataRead, readByteSize, &sd.bytesread) != FR_OK)
                        errorCount += 1;
                    for (uint16_t j = 0; j < readByteSize; j++) {
                        volatile uint8_t *writePtr = (volatile uint8_t *)(data + pointerOffset);
                        *writePtr = dataRead[j];
                        pointerOffset += 1;
                    }
                    memset(dataRead, 0x00, readByteSize);
                }

                if (remainderByteSize) {
                    if (f_read(&sd.file, &dataRead, remainderByteSize, &sd.bytesread) != FR_OK)
                        errorCount += 1;
                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                        volatile uint8_t *writePtr = (volatile uint8_t *)(data + pointerOffset);
                        *writePtr = dataRead[k];
                        pointerOffset += 1;
                    }
                }

                f_close(&sd.file);
                if (errorCount == 0) {
                    // write wavetableData
                    bool wavetableFile[2];
                    char wavetableName[2][kFileNameSize + 1];
                    int16_t wavetableNum[2];
                    bool wavetableMissing[2] = {false};
                    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
                        uint16_t wavetableOffset = 200 + (i * 50);
                        data[wavetableOffset] == 'x' ? wavetableFile[i] = true : wavetableFile[i] = false;

                        if (wavetableFile[i]) {
                            strncpy(wavetableName[i], &data[wavetableOffset + 1], 32);
                            if ((f_open(&sd.file, "System/Data/Wavetable.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, strlen(kFileRef)) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                int16_t searchCounter = 0;

                                while (searchActive) {
                                    memset(searchName, 0x00, sizeof(searchName));
                                    f_read(&sd.file, searchName, kFileNameSize, &sd.bytesread);

                                    if (strcmp(searchName, wavetableName[i]) == 0) {
                                        wavetableNum[i] = searchCounter;
                                        wavetableMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= wavetableLibrarySize) {
                                        wavetableNum[i] = -1;
                                        wavetableMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                wavetableMissing[i] = true;
                            }
                            f_close(&sd.file);
                            HAL_Delay(50);
                        } else {
                            wavetableNum[i] = -1;
                            wavetableMissing[i] = false;
                        }
                        if (wavetableMissing[i]) {
                            osc_setWavetableSelected(osc[i], -1);
                            osc_setWavetableLoaded(osc[i], false);
                            osc_setActive(osc[i], false);
                        } else {
                            osc_setWavetableSelected(osc[i], wavetableNum[i]);
                            osc_setWavetableLoaded(osc[i], false);
                        }
                    }
                    // write rhythmData
                    rhythm_setTempo(data[1]);
                    rhythm_setMeasure(data[2]);
                    rhythm_setBar(data[3]);
                    rhythm_setQuantize(data[4]);
                    // write metronomeData
                    metro_setActive((bool)data[5]);
                    metro_setPrecount(data[6]);
                    metro_setSample(data[7]);
                    metro_setVolume(data[8]);
                    // write keyData
                    key_setNote(data[9]);
                    key_setArpeg(data[10]);
                    key_setRate(data[11]);
                    key_setOsc(data[12]);
                    key_setChord(data[13]);
                    key_setOrder(data[14]);
                    key_setOctave(data[15]);
                    // write osc0Data
                    osc_setActive(osc[0], (bool)data[16]);
                    osc_setLevel(osc[0], data[17]);
                    osc_setTune(osc[0], data[18]);
                    osc_setPhase(osc[0], data[19]);
                    osc_setNormalize(osc[0], data[20]);
                    osc_setStart(osc[0], data[21]);
                    osc_setEnd(osc[0], data[22]);
                    osc_setXFlip(osc[0], data[23]);
                    osc_setYFlip(osc[0], data[24]);
                    osc_lfo_setActive(osc[0], osc[0].lfo[0], (bool)data[25]);
                    osc_lfo_setType(osc[0], osc[0].lfo[0], data[26]);
                    osc_lfo_setTarget(osc[0], osc[0].lfo[0], data[27]);
                    osc_lfo_setRate(osc[0], osc[0].lfo[0], data[28]);
                    osc_lfo_setDepth(osc[0], osc[0].lfo[0], data[29]);
                    osc_lfo_setLoop(osc[0], osc[0].lfo[0], data[30]);
                    osc_lfo_setActive(osc[0], osc[0].lfo[1], (bool)data[31]);
                    osc_lfo_setType(osc[0], osc[0].lfo[1], data[32]);
                    osc_lfo_setTarget(osc[0], osc[0].lfo[1], data[33]);
                    osc_lfo_setRate(osc[0], osc[0].lfo[1], data[34]);
                    osc_lfo_setDepth(osc[0], osc[0].lfo[1], data[35]);
                    osc_lfo_setLoop(osc[0], osc[0].lfo[1], data[36]);
                    // write osc1Data
                    osc_setActive(osc[1], (bool)data[37]);
                    osc_setLevel(osc[1], data[38]);
                    osc_setTune(osc[1], data[39]);
                    osc_setPhase(osc[1], data[40]);
                    osc_setNormalize(osc[1], data[41]);
                    osc_setStart(osc[1], data[42]);
                    osc_setEnd(osc[1], data[43]);
                    osc_setXFlip(osc[1], data[44]);
                    osc_setYFlip(osc[1], data[45]);
                    osc_lfo_setActive(osc[1], osc[1].lfo[0], (bool)data[46]);
                    osc_lfo_setType(osc[1], osc[1].lfo[0], data[47]);
                    osc_lfo_setTarget(osc[1], osc[1].lfo[0], data[48]);
                    osc_lfo_setRate(osc[1], osc[1].lfo[0], data[49]);
                    osc_lfo_setDepth(osc[1], osc[1].lfo[0], data[50]);
                    osc_lfo_setLoop(osc[1], osc[1].lfo[0], data[51]);
                    osc_lfo_setActive(osc[1], osc[1].lfo[1], (bool)data[52]);
                    osc_lfo_setType(osc[1], osc[1].lfo[1], data[53]);
                    osc_lfo_setTarget(osc[1], osc[1].lfo[1], data[54]);
                    osc_lfo_setRate(osc[1], osc[1].lfo[1], data[55]);
                    osc_lfo_setDepth(osc[1], osc[1].lfo[1], data[56]);
                    osc_lfo_setLoop(osc[1], osc[1].lfo[1], data[57]);
                    // write eqData
                    eq_setActive((bool)data[58]);
                    eq_setFreqLowShelf(data[59]);
                    eq_setGainLowShelf(data[60]);
                    eq_setFreqHighShelf(data[61]);
                    eq_setGainHighShelf(data[62]);
                    eq_setQPeak(0, data[63]);
                    eq_setQPeak(1, data[64]);
                    eq_setQPeak(2, data[65]);
                    eq_setQPeak(3, data[66]);
                    eq_setFreqPeak(0, data[67]);
                    eq_setFreqPeak(1, data[68]);
                    eq_setFreqPeak(2, data[69]);
                    eq_setFreqPeak(3, data[70]);
                    eq_setGainPeak(0, data[71]);
                    eq_setGainPeak(1, data[72]);
                    eq_setGainPeak(2, data[73]);
                    eq_setGainPeak(3, data[74]);
                    // write filterData
                    filter_setActive(0, (bool)data[75]);
                    filter_setType(0, data[76]);
                    filter_setFreq(0, data[77]);
                    filter_setRes(0, data[78]);
                    filter_setSlope(0, data[79]);
                    filter_setDry(0, data[80]);
                    filter_setWet(0, data[81]);
                    filter_setActive(1, (bool)data[82]);
                    filter_setType(1, data[83]);
                    filter_setFreq(1, data[84]);
                    filter_setRes(1, data[85]);
                    filter_setSlope(1, data[86]);
                    filter_setDry(1, data[87]);
                    filter_setWet(1, data[88]);
                    // write envelopeData
                    envelope_setActive((bool)data[89]);
                    envelope_setType(data[90]);
                    envelope_setCurve(data[91]);
                    envelope_setAttackTime(data[92]);
                    envelope_setDecayTime(data[93]);
                    envelope_setSustainLevel(data[94]);
                    envelope_setReleaseTime(data[95]);
                    // write effectData
                    effect_setActive(0, (bool)data[96]);
                    effect_setType(0, data[97]);
                    effect_setActive(1, (bool)data[98]);
                    effect_setType(1, data[99]);
                    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
                        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
                            uint16_t baseNum = 100 + (45 * i) + (5 * j);
                            effect_setAData(i, j, data[baseNum + 0]);
                            effect_setBData(i, j, data[baseNum + 1]);
                            effect_setCData(i, j, data[baseNum + 2]);
                            effect_setDData(i, j, data[baseNum + 3]);
                            effect_setEData(i, j, data[baseNum + 4]);
                        }
                    }
                    // write reverbData
                    reverb_setActive(data[190]);
                    reverb_setSize(data[191]);
                    reverb_setDecay(data[192]);
                    reverb_setPreDelay(data[193]);
                    reverb_setSurround(data[194]);
                    reverb_setDry(data[195]);
                    reverb_setWet(data[196]);

                    for (uint8_t j = 0; j < kOscLibrarySize; j++) {
                        lcd_drawInfo_Osc_WavetableData(osc[j]);
                        lcd_drawInfo_Osc_GraphData(osc[j]);
                    }
                    // write songData
                    uint16_t offsetSong = 300;


                    
                    for (uint8_t i = 0; i < kBankLibrarySize; i++) {
                        Bank &bank_ = song.bankLibrary[i];
                        uint16_t offsetBank = offsetSong + 1 + (i * 641);
                        for (uint8_t j = 0; j < kBeatLibrarySize; j++) {
                            uint16_t offsetBeat = offsetBank + (8 * j);
                            bool active = data[offsetBeat + 0];
                            if (active) {
                                uint8_t octave_ = data[offsetBeat + 2];
                                uint8_t note_ = data[offsetBeat + 3];
                                uint16_t startInterval_ = (data[offsetBeat + 5] << 8) | (data[offsetBeat + 4]);
                                uint16_t endInterval_ = (data[offsetBeat + 7] << 8) | (data[offsetBeat + 6]);
                                song_setBeat(i, startInterval_, endInterval_, octave_, note_);
                            }
                        }
                    }

                    uint8_t missingWavetable = 0;
                    for (uint8_t j = 0; j < kOscLibrarySize; j++) {
                        if (wavetableMissing[j])
                            missingWavetable += 1;
                    }
                    if (missingWavetable) {
                        (missingWavetable == 1) ? alertType = ALERT_MISSINGWAVETABLE : alertType = ALERT_MISSINGWAVETABLES;
                        lcd_drawAlert();
                        HAL_Delay(1000);
                        result = SD_ERROR;
                    } else {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_saveFile(uint8_t fileNum_) {
    SdResult result = SD_ERROR;

    char eof[] = "EOF";
    char fileName[50];
    char fileNum[4];
    sprintf(fileNum, "%03d", (fileNum_ + 1));
    strcpy(fileName, kFileStart);
    strcat(fileName, fileNum);
    strcat(fileName, kFileEnd);

    // 000.000 *              0001 byte
    // 000.001 main           0196 bytes
    // 000.200 wavetable      0200 bytes
    // 000.300 song           0100 bytes
    // total size             4788 bytes

    // write file
    uint8_t data[kFileByteSize] = {};
    data[0] = '*';
    // write rhythmData
    data[1] = rhythm.tempo;
    data[2] = rhythm.measure;
    data[3] = rhythm.bar;
    data[4] = rhythm.quantize;
    // write metronomeData
    data[5] = metronome.active;
    data[6] = metronome.precount;
    data[7] = metronome.sample;
    data[8] = metronome.volume;
    // write keyData
    data[9] = key.note;
    data[10] = key.arpeg;
    data[11] = key.rate;
    data[12] = key.osc;
    data[13] = key.chord;
    data[14] = key.order;
    data[15] = key.octave;
    // write osc0Data
    data[16] = osc[0].active;
    data[17] = osc[0].level;
    data[18] = osc[0].tune;
    data[19] = osc[0].phase;
    data[20] = osc[0].normalize;
    data[21] = osc[0].start;
    data[22] = osc[0].end;
    data[23] = osc[0].xFlip;
    data[24] = osc[0].yFlip;
    data[25] = osc[0].lfo[0].active;
    data[26] = osc[0].lfo[0].type;
    data[27] = osc[0].lfo[0].target;
    data[28] = osc[0].lfo[0].rate;
    data[29] = osc[0].lfo[0].depth;
    data[30] = osc[0].lfo[0].loop;
    data[31] = osc[0].lfo[1].active;
    data[32] = osc[0].lfo[1].type;
    data[33] = osc[0].lfo[1].target;
    data[34] = osc[0].lfo[1].rate;
    data[35] = osc[0].lfo[1].depth;
    data[36] = osc[0].lfo[1].loop;
    // write osc1Data
    data[37] = osc[1].active;
    data[38] = osc[1].level;
    data[39] = osc[1].tune;
    data[40] = osc[1].phase;
    data[41] = osc[1].normalize;
    data[42] = osc[1].start;
    data[43] = osc[1].end;
    data[44] = osc[1].xFlip;
    data[45] = osc[1].yFlip;
    data[46] = osc[1].lfo[0].active;
    data[47] = osc[1].lfo[0].type;
    data[48] = osc[1].lfo[0].target;
    data[49] = osc[1].lfo[0].rate;
    data[50] = osc[1].lfo[0].depth;
    data[51] = osc[1].lfo[0].loop;
    data[52] = osc[1].lfo[1].active;
    data[53] = osc[1].lfo[1].type;
    data[54] = osc[1].lfo[1].target;
    data[55] = osc[1].lfo[1].rate;
    data[56] = osc[1].lfo[1].depth;
    data[57] = osc[1].lfo[1].loop;
    // write eqData
    data[58] = eq.active;
    data[59] = eq.freqLowShelf;
    data[60] = eq.gainLowShelf;
    data[61] = eq.freqHighShelf;
    data[62] = eq.gainHighShelf;
    data[63] = eq.qPeak[0];
    data[64] = eq.qPeak[1];
    data[65] = eq.qPeak[2];
    data[66] = eq.qPeak[3];
    data[67] = eq.freqPeak[0];
    data[68] = eq.freqPeak[1];
    data[69] = eq.freqPeak[2];
    data[70] = eq.freqPeak[3];
    data[71] = eq.gainPeak[0];
    data[72] = eq.gainPeak[1];
    data[73] = eq.gainPeak[2];
    data[74] = eq.gainPeak[3];
    // write filterData
    data[75] = filter[0].active;
    data[76] = filter[0].type;
    data[77] = filter[0].freq;
    data[78] = filter[0].res;
    data[79] = filter[0].slope;
    data[80] = filter[0].dry;
    data[81] = filter[0].wet;
    data[82] = filter[1].active;
    data[83] = filter[1].type;
    data[84] = filter[1].freq;
    data[85] = filter[1].res;
    data[86] = filter[1].slope;
    data[87] = filter[1].dry;
    data[88] = filter[1].wet;
    // write envelopeData
    data[89] = envelope.active;
    data[90] = envelope.type;
    data[91] = envelope.curve;
    data[92] = envelope.attackTime;
    data[93] = envelope.decayTime;
    data[94] = envelope.sustainLevel;
    data[95] = envelope.releaseTime;
    // write effectData
    data[96] = effect[0].active;
    data[97] = effect[0].type;
    data[98] = effect[1].active;
    data[99] = effect[1].type;
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
            uint16_t baseNum = 100 + (45 * i) + (5 * j);
            data[baseNum + 0] = effect[i].subEffect[j].aData;
            data[baseNum + 1] = effect[i].subEffect[j].bData;
            data[baseNum + 2] = effect[i].subEffect[j].cData;
            data[baseNum + 3] = effect[i].subEffect[j].dData;
            data[baseNum + 4] = effect[i].subEffect[j].eData;
        }
    }
    // write reverbData
    data[190] = reverb.active;
    data[191] = reverb.size;
    data[192] = reverb.decay;
    data[193] = reverb.preDelay;
    data[194] = reverb.surround;
    data[195] = reverb.dry;
    data[196] = reverb.wet;
    // write wavetableData
    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
        uint16_t wavetableOffset = 200 + (i * 50);
        if (osc[i].wavetableLoaded != -1) {
            data[wavetableOffset] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[wavetableOffset + 1 + j] = osc[i].wavetableLoadedData.nameLongR[j];
            }
        } else {
            data[wavetableOffset] = 'o';
        }
    }
    // write songData
    uint16_t offsetSong = 300;
    data[offsetSong] = 's';
    for (uint8_t i = 0; i < kBankLibrarySize; i++) {
        Bank &bank_ = song.bankLibrary[i];
        uint16_t offsetBank = offsetSong + 1 + (i * 641);
        data[offsetBank] = 'b';
        for (uint8_t j = 0; j < kBeatLibrarySize; j++) {
            Beat &beat_ = bank_.beatLibrary[j];
            uint16_t offsetBeat = offsetBank + (8 * j);
            data[offsetBeat + 0] = beat_.active;
            data[offsetBeat + 1] = false;
            data[offsetBeat + 2] = beat_.octave;
            data[offsetBeat + 3] = beat_.note;
            data[offsetBeat + 4] = beat_.startInterval & 0xFF;
            data[offsetBeat + 5] = beat_.startInterval >> 8;
            data[offsetBeat + 6] = beat_.endInterval & 0xFF;
            data[offsetBeat + 7] = beat_.endInterval >> 8;
        }
    }
    // disable keyboard
    keyboard_disable();
    // write data to sdcard
    if (sd_checkFile(fileNum_) == SD_OK) {
        if (f_open(&sd.file, fileName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (f_write(&sd.file, kFileRef, strlen(kFileRef), &sd.byteswritten) == FR_OK) {
                const uint16_t writeByteSize = 512;
                const uint16_t fileByteSize = kFileByteSize;
                uint16_t writeCountTotal = (fileByteSize / writeByteSize);
                uint16_t remainderByteSize = (fileByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (f_write(&sd.file, (data + pointerOffset), writeByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                    pointerOffset += 512;
                }

                if (remainderByteSize) {
                    if (f_write(&sd.file, (data + pointerOffset), remainderByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                }

                if (errorCount == 0) {
                    if (f_write(&sd.file, eof, strlen(eof), &sd.byteswritten) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_clearFile(uint8_t fileNum_) {
    SdResult result = SD_ERROR;

    char eof[] = "EOF";
    char fileName[50];
    char fileNum[4];
    sprintf(fileNum, "%03d", (fileNum_ + 1));
    strcpy(fileName, kFileStart);
    strcat(fileName, fileNum);
    strcat(fileName, kFileEnd);

    // 000.000 *              0001 byte
    // 000.001 main           0177 bytes
    // 000.200 wavetable      0200 bytes
    // 000.300 song           0100 bytes
    // total size             4788 bytes

    const char data[kFileByteSize + 1] =
        "-                                                                       "
        "                                                                        "
        "                                                        o               "
        "                                  o                                     "
        "            sb                                                          "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "      b                                                                 "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                       "
        "b                                                                       "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                 b      "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                          b             "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                   b                    "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                            b                           "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                                                        "
        "                                     ";

    // disable keyboard
    keyboard_disable();
    if (sd_checkFile(fileNum_) == SD_OK) {
        if (f_open(&sd.file, fileName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (f_write(&sd.file, kFileRef, strlen(kFileRef), &sd.byteswritten) == FR_OK) {
                const uint16_t writeByteSize = 512;
                const uint16_t fileByteSize = kFileByteSize;
                uint16_t writeCountTotal = (fileByteSize / writeByteSize);
                uint16_t remainderByteSize = (fileByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (f_write(&sd.file, (data + pointerOffset), writeByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                    pointerOffset += 512;
                }

                if (remainderByteSize) {
                    if (f_write(&sd.file, (data + pointerOffset), remainderByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                }

                if (errorCount == 0) {
                    if (f_write(&sd.file, eof, strlen(eof), &sd.byteswritten) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_checkSynthkit(uint8_t kitNum_) {
    SdResult result = SD_ERROR;
    synthkitStatus = FILE_NONE;
    char refRead[sizeof(kSynthkitRef)] = {};
    char data;

    char synthkitName[50];
    char synthkitNum[4];
    sprintf(synthkitNum, "%03d", (kitNum_ + 1));
    strcpy(synthkitName, kSynthkitStart);
    strcat(synthkitName, synthkitNum);
    strcat(synthkitName, kSynthkitEnd);

    if (f_open(&sd.file, synthkitName, FA_READ) == FR_OK) {
        if (f_read(&sd.file, refRead, strlen(kSynthkitRef), &sd.bytesread) == FR_OK) {
            if (strncmp(kSynthkitRef, refRead, strlen(kSynthkitRef)) == 0) {
                f_read(&sd.file, &data, 1, &sd.bytesread);
                (data == '*') ? synthkitStatus = FILE_ACTIVE : synthkitStatus = FILE_INACTIVE;
            } else {
                synthkitStatus = FILE_INCOMPATIBLE;
            }
        } else {
            synthkitStatus = FILE_INCOMPATIBLE;
        }
    } else {
        synthkitStatus = FILE_MISSING;
    }
    f_close(&sd.file);
    result = SD_OK;
    return result;
}

SdResult Controller::sd_loadSynthkit(bool mode_, uint8_t kitNum_) {
    SdResult result = SD_ERROR;

    char synthkitName[50];
    char synthkitNum[4];
    sprintf(synthkitNum, "%03d", (kitNum_ + 1));
    strcpy(synthkitName, kSynthkitStart);
    strcat(synthkitName, synthkitNum);
    strcat(synthkitName, kSynthkitEnd);

    // 000.000 *               001 byte
    // 000.001 main            188 bytes
    // 000.200 wavetable       100 bytes
    // total size              300 bytes

    // disable keyboard
    keyboard_disable();
    // load synthkit
    if ((sd_checkSynthkit(kitNum_) == SD_OK) && (synthkitStatus == FILE_ACTIVE)) {
        char data[kSynthkitByteSize] = {0x00};
        if (f_open(&sd.file, synthkitName, FA_READ) == FR_OK) {
            if (f_lseek(&sd.file, strlen(kSynthkitRef)) == FR_OK) {
                const uint16_t readByteSize = 512;
                const uint16_t synthkitByteSize = kSynthkitByteSize;
                uint16_t readCountTotal = (synthkitByteSize / readByteSize);
                uint16_t remainderByteSize = (synthkitByteSize % readByteSize);
                uint8_t dataRead[readByteSize] = {0};
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < readCountTotal; i++) {
                    if (f_read(&sd.file, &dataRead, readByteSize, &sd.bytesread) != FR_OK)
                        errorCount += 1;
                    for (uint16_t j = 0; j < readByteSize; j++) {
                        volatile uint8_t *writePtr = (volatile uint8_t *)(data + pointerOffset);
                        *writePtr = dataRead[j];
                        pointerOffset += 1;
                    }
                    memset(dataRead, 0x00, readByteSize);
                }

                if (remainderByteSize) {
                    if (f_read(&sd.file, &dataRead, remainderByteSize, &sd.bytesread) != FR_OK)
                        errorCount += 1;
                    for (uint16_t k = 0; k < remainderByteSize; k++) {
                        volatile uint8_t *writePtr = (volatile uint8_t *)(data + pointerOffset);
                        *writePtr = dataRead[k];
                        pointerOffset += 1;
                    }
                }

                f_close(&sd.file);
                if (errorCount == 0) {
                    // write wavetableData
                    bool wavetableFile[2];
                    char wavetableName[2][kFileNameSize + 1];
                    int16_t wavetableNum[2];
                    bool wavetableMissing[2] = {false};
                    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
                        uint16_t wavetableOffset = 200 + (i * 50);
                        data[wavetableOffset] == 'x' ? wavetableFile[i] = true : wavetableFile[i] = false;

                        if (wavetableFile[i]) {
                            strncpy(wavetableName[i], &data[wavetableOffset + 1], 32);
                            if ((f_open(&sd.file, "System/Data/Wavetable.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, strlen(kFileRef)) == FR_OK)) {
                                char searchName[kFileNameSize + 1];
                                bool searchActive = true;
                                int16_t searchCounter = 0;

                                while (searchActive) {
                                    memset(searchName, 0x00, kFileNameSize);
                                    f_read(&sd.file, searchName, kFileNameSize, &sd.bytesread);

                                    if (strcmp(searchName, wavetableName[i]) == 0) {
                                        wavetableNum[i] = searchCounter;
                                        wavetableMissing[i] = false;
                                        searchActive = false;
                                    } else if (searchCounter >= wavetableLibrarySize) {
                                        wavetableNum[i] = -1;
                                        wavetableMissing[i] = true;
                                        searchActive = false;
                                    }
                                    searchCounter += 1;
                                }
                            } else {
                                wavetableMissing[i] = true;
                            }
                            f_close(&sd.file);
                            HAL_Delay(50);
                        } else {
                            wavetableNum[i] = -1;
                            wavetableMissing[i] = false;
                        }
                        if (wavetableMissing[i]) {
                            osc_setWavetableSelected(osc[i], -1);
                            osc_setWavetableLoaded(osc[i], false);
                            osc_setActive(osc[i], false);
                        } else {
                            osc_setWavetableSelected(osc[i], wavetableNum[i]);
                            osc_setWavetableLoaded(osc[i], false);
                        }
                    }
                    // write keyData
                    key_setNote(data[1]);
                    key_setArpeg(data[2]);
                    key_setRate(data[3]);
                    key_setOsc(data[4]);
                    key_setChord(data[5]);
                    key_setOrder(data[6]);
                    key_setOctave(data[7]);
                    // write osc0Data
                    osc_setActive(osc[0], (bool)data[8]);
                    osc_setLevel(osc[0], data[9]);
                    osc_setTune(osc[0], data[10]);
                    osc_setPhase(osc[0], data[11]);
                    osc_setNormalize(osc[0], data[12]);
                    osc_setStart(osc[0], data[13]);
                    osc_setEnd(osc[0], data[14]);
                    osc_setXFlip(osc[0], data[15]);
                    osc_setYFlip(osc[0], data[16]);
                    osc_lfo_setActive(osc[0], osc[0].lfo[0], (bool)data[17]);
                    osc_lfo_setType(osc[0], osc[0].lfo[0], data[18]);
                    osc_lfo_setTarget(osc[0], osc[0].lfo[0], data[19]);
                    osc_lfo_setRate(osc[0], osc[0].lfo[0], data[20]);
                    osc_lfo_setDepth(osc[0], osc[0].lfo[0], data[21]);
                    osc_lfo_setLoop(osc[0], osc[0].lfo[0], data[22]);
                    osc_lfo_setActive(osc[0], osc[0].lfo[1], (bool)data[23]);
                    osc_lfo_setType(osc[0], osc[0].lfo[1], data[24]);
                    osc_lfo_setTarget(osc[0], osc[0].lfo[1], data[25]);
                    osc_lfo_setRate(osc[0], osc[0].lfo[1], data[26]);
                    osc_lfo_setDepth(osc[0], osc[0].lfo[1], data[27]);
                    osc_lfo_setLoop(osc[0], osc[0].lfo[1], data[28]);
                    // write osc1Data
                    osc_setActive(osc[1], (bool)data[29]);
                    osc_setLevel(osc[1], data[30]);
                    osc_setTune(osc[1], data[31]);
                    osc_setPhase(osc[1], data[32]);
                    osc_setNormalize(osc[1], data[33]);
                    osc_setStart(osc[1], data[34]);
                    osc_setEnd(osc[1], data[35]);
                    osc_setXFlip(osc[1], data[36]);
                    osc_setYFlip(osc[1], data[37]);
                    osc_lfo_setActive(osc[1], osc[1].lfo[0], (bool)data[38]);
                    osc_lfo_setType(osc[1], osc[1].lfo[0], data[39]);
                    osc_lfo_setTarget(osc[1], osc[1].lfo[0], data[40]);
                    osc_lfo_setRate(osc[1], osc[1].lfo[0], data[41]);
                    osc_lfo_setDepth(osc[1], osc[1].lfo[0], data[42]);
                    osc_lfo_setLoop(osc[1], osc[1].lfo[0], data[43]);
                    osc_lfo_setActive(osc[1], osc[1].lfo[1], (bool)data[44]);
                    osc_lfo_setType(osc[1], osc[1].lfo[1], data[45]);
                    osc_lfo_setTarget(osc[1], osc[1].lfo[1], data[46]);
                    osc_lfo_setRate(osc[1], osc[1].lfo[1], data[47]);
                    osc_lfo_setDepth(osc[1], osc[1].lfo[1], data[48]);
                    osc_lfo_setLoop(osc[1], osc[1].lfo[1], data[49]);
                    // write eqData
                    eq_setActive((bool)data[50]);
                    eq_setFreqLowShelf(data[51]);
                    eq_setGainLowShelf(data[52]);
                    eq_setFreqHighShelf(data[53]);
                    eq_setGainHighShelf(data[54]);
                    eq_setQPeak(0, data[55]);
                    eq_setQPeak(1, data[56]);
                    eq_setQPeak(2, data[57]);
                    eq_setQPeak(3, data[58]);
                    eq_setFreqPeak(0, data[59]);
                    eq_setFreqPeak(1, data[60]);
                    eq_setFreqPeak(2, data[61]);
                    eq_setFreqPeak(3, data[62]);
                    eq_setGainPeak(0, data[63]);
                    eq_setGainPeak(1, data[64]);
                    eq_setGainPeak(2, data[65]);
                    eq_setGainPeak(3, data[66]);
                    // write filterData
                    filter_setActive(0, (bool)data[67]);
                    filter_setType(0, data[68]);
                    filter_setFreq(0, data[69]);
                    filter_setRes(0, data[70]);
                    filter_setSlope(0, data[71]);
                    filter_setDry(0, data[72]);
                    filter_setWet(0, data[73]);
                    filter_setActive(1, (bool)data[74]);
                    filter_setType(1, data[75]);
                    filter_setFreq(1, data[76]);
                    filter_setRes(1, data[77]);
                    filter_setSlope(1, data[78]);
                    filter_setDry(1, data[79]);
                    filter_setWet(1, data[80]);
                    // write envelopeData
                    envelope_setActive((bool)data[81]);
                    envelope_setType(data[82]);
                    envelope_setCurve(data[83]);
                    envelope_setAttackTime(data[84]);
                    envelope_setDecayTime(data[85]);
                    envelope_setSustainLevel(data[86]);
                    envelope_setReleaseTime(data[87]);
                    // write effectData
                    effect_setActive(0, (bool)data[88]);
                    effect_setType(0, data[89]);
                    effect_setActive(1, (bool)data[90]);
                    effect_setType(1, data[91]);
                    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
                        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
                            uint16_t baseNum = 92 + (45 * i) + (5 * j);
                            effect_setAData(i, j, data[baseNum + 0]);
                            effect_setBData(i, j, data[baseNum + 1]);
                            effect_setCData(i, j, data[baseNum + 2]);
                            effect_setDData(i, j, data[baseNum + 3]);
                            effect_setEData(i, j, data[baseNum + 4]);
                        }
                    }
                    // write reverbData
                    reverb_setActive(data[182]);
                    reverb_setSize(data[183]);
                    reverb_setDecay(data[184]);
                    reverb_setPreDelay(data[185]);
                    reverb_setSurround(data[186]);
                    reverb_setDry(data[187]);
                    reverb_setWet(data[188]);

                    for (uint8_t j = 0; j < kOscLibrarySize; j++) {
                        lcd_drawInfo_Osc_WavetableData(osc[j]);
                        lcd_drawInfo_Osc_GraphData(osc[j]);
                    }

                    uint8_t missingWavetable = 0;
                    for (uint8_t j = 0; j < kOscLibrarySize; j++) {
                        if (wavetableMissing[j])
                            missingWavetable += 1;
                    }
                    if (missingWavetable) {
                        (missingWavetable == 1) ? alertType = ALERT_MISSINGWAVETABLE : alertType = ALERT_MISSINGWAVETABLES;
                        lcd_drawAlert();
                        HAL_Delay(1000);
                        result = SD_ERROR;
                    } else {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_saveSynthkit(bool mode_, uint8_t kitNum_) {
    SdResult result = SD_ERROR;

    char eof[] = "EOF";
    char synthkitName[50];
    char synthkitNum[4];
    sprintf(synthkitNum, "%03d", (kitNum_ + 1));
    strcpy(synthkitName, kSynthkitStart);
    strcat(synthkitName, synthkitNum);
    strcat(synthkitName, kSynthkitEnd);

    // 000.000 *               001 byte
    // 000.001 main            188 bytes
    // 000.200 wavetable       100 bytes
    // total size              300 bytes

    // write synthkit
    uint8_t data[kSynthkitByteSize] = {0x00};
    data[0] = '*';
    // write keyData
    data[1] = key.note;
    data[2] = key.arpeg;
    data[3] = key.rate;
    data[4] = key.osc;
    data[5] = key.chord;
    data[6] = key.order;
    data[7] = key.octave;
    // write osc0Data
    data[8] = osc[0].active;
    data[9] = osc[0].level;
    data[10] = osc[0].tune;
    data[11] = osc[0].phase;
    data[12] = osc[0].normalize;
    data[13] = osc[0].start;
    data[14] = osc[0].end;
    data[15] = osc[0].xFlip;
    data[16] = osc[0].yFlip;
    data[17] = osc[0].lfo[0].active;
    data[18] = osc[0].lfo[0].type;
    data[19] = osc[0].lfo[0].target;
    data[20] = osc[0].lfo[0].rate;
    data[21] = osc[0].lfo[0].depth;
    data[22] = osc[0].lfo[0].loop;
    data[23] = osc[0].lfo[1].active;
    data[24] = osc[0].lfo[1].type;
    data[25] = osc[0].lfo[1].target;
    data[26] = osc[0].lfo[1].rate;
    data[27] = osc[0].lfo[1].depth;
    data[28] = osc[0].lfo[1].loop;
    // write osc1Data
    data[29] = osc[1].active;
    data[30] = osc[1].level;
    data[31] = osc[1].tune;
    data[32] = osc[1].phase;
    data[33] = osc[1].normalize;
    data[34] = osc[1].start;
    data[35] = osc[1].end;
    data[36] = osc[1].xFlip;
    data[37] = osc[1].yFlip;
    data[38] = osc[1].lfo[0].active;
    data[39] = osc[1].lfo[0].type;
    data[40] = osc[1].lfo[0].target;
    data[41] = osc[1].lfo[0].rate;
    data[42] = osc[1].lfo[0].depth;
    data[43] = osc[1].lfo[0].loop;
    data[44] = osc[1].lfo[1].active;
    data[45] = osc[1].lfo[1].type;
    data[46] = osc[1].lfo[1].target;
    data[47] = osc[1].lfo[1].rate;
    data[48] = osc[1].lfo[1].depth;
    data[49] = osc[1].lfo[1].loop;
    // write eqData
    data[50] = eq.active;
    data[51] = eq.freqLowShelf;
    data[52] = eq.gainLowShelf;
    data[53] = eq.freqHighShelf;
    data[54] = eq.gainHighShelf;
    data[55] = eq.qPeak[0];
    data[56] = eq.qPeak[1];
    data[57] = eq.qPeak[2];
    data[58] = eq.qPeak[3];
    data[59] = eq.freqPeak[0];
    data[60] = eq.freqPeak[1];
    data[61] = eq.freqPeak[2];
    data[62] = eq.freqPeak[3];
    data[63] = eq.gainPeak[0];
    data[64] = eq.gainPeak[1];
    data[65] = eq.gainPeak[2];
    data[66] = eq.gainPeak[3];
    // write filterData
    data[67] = filter[0].active;
    data[68] = filter[0].type;
    data[69] = filter[0].freq;
    data[70] = filter[0].res;
    data[71] = filter[0].slope;
    data[72] = filter[0].dry;
    data[73] = filter[0].wet;
    data[74] = filter[1].active;
    data[75] = filter[1].type;
    data[76] = filter[1].freq;
    data[77] = filter[1].res;
    data[78] = filter[1].slope;
    data[79] = filter[1].dry;
    data[80] = filter[1].wet;
    // write envelopeData
    data[81] = envelope.active;
    data[82] = envelope.type;
    data[83] = envelope.curve;
    data[84] = envelope.attackTime;
    data[85] = envelope.decayTime;
    data[86] = envelope.sustainLevel;
    data[87] = envelope.releaseTime;
    // write effectData
    data[88] = effect[0].active;
    data[89] = effect[0].type;
    data[90] = effect[1].active;
    data[91] = effect[1].type;
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        for (uint8_t j = 0; j < kSubEffectLibrarySize; j++) {
            uint16_t baseNum = 92 + (45 * i) + (5 * j);
            data[baseNum + 0] = effect[i].subEffect[j].aData;
            data[baseNum + 1] = effect[i].subEffect[j].bData;
            data[baseNum + 2] = effect[i].subEffect[j].cData;
            data[baseNum + 3] = effect[i].subEffect[j].dData;
            data[baseNum + 4] = effect[i].subEffect[j].eData;
        }
    }
    // write reverbData
    data[182] = reverb.active;
    data[183] = reverb.size;
    data[184] = reverb.decay;
    data[185] = reverb.preDelay;
    data[186] = reverb.surround;
    data[187] = reverb.dry;
    data[188] = reverb.wet;

    // write wavetableData
    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
        uint16_t wavetableOffset = 200 + (i * 50);
        if (osc[i].wavetableLoaded != -1) {
            data[wavetableOffset] = 'x';
            for (uint8_t j = 0; j < kFileNameSize; j++) {
                data[wavetableOffset + 1 + j] = osc[i].wavetableLoadedData.nameLongR[j];
            }
        } else {
            data[wavetableOffset] = 'o';
        }
    }
    // disable keyboard
    keyboard_disable();
    // write data to sdcard
    if (sd_checkSynthkit(kitNum_) == SD_OK) {
        if (f_open(&sd.file, synthkitName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (f_write(&sd.file, kSynthkitRef, strlen(kSynthkitRef), &sd.byteswritten) == FR_OK) {
                const uint16_t writeByteSize = 512;
                const uint16_t synthkitByteSize = kSynthkitByteSize;
                uint16_t writeCountTotal = (synthkitByteSize / writeByteSize);
                uint16_t remainderByteSize = (synthkitByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (f_write(&sd.file, (data + pointerOffset), writeByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                    pointerOffset += 512;
                }

                if (remainderByteSize) {
                    if (f_write(&sd.file, (data + pointerOffset), remainderByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                }

                if (errorCount == 0) {
                    if (f_write(&sd.file, eof, 3, &sd.byteswritten) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_clearSynthkit(bool mode_, uint8_t kitNum_) {
    SdResult result = SD_ERROR;

    char eof[] = "EOF";
    char synthkitName[50];
    char synthkitNum[4];
    sprintf(synthkitNum, "%03d", (kitNum_ + 1));
    strcpy(synthkitName, kSynthkitStart);
    strcat(synthkitName, synthkitNum);
    strcat(synthkitName, kSynthkitEnd);

    // 000.000 *               001 byte
    // 000.001 main            177 bytes
    // 000.200 wavetable       100 bytes
    // total size              300 bytes

    const char data[] =
        "-                                                                       "
        "                                                                        "
        "                                                        o               "
        "                                  o                                     "
        "            ";

    // disable keyboard
    keyboard_disable();
    // clear synthkit
    if (sd_checkSynthkit(kitNum_) == SD_OK) {
        if (f_open(&sd.file, synthkitName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
            if (f_write(&sd.file, kSynthkitRef, strlen(kSynthkitRef), &sd.byteswritten) == FR_OK) {
                const uint16_t writeByteSize = 512;
                const uint16_t synthkitByteSize = kSynthkitByteSize;
                uint16_t writeCountTotal = (synthkitByteSize / writeByteSize);
                uint16_t remainderByteSize = (synthkitByteSize % writeByteSize);
                uint32_t pointerOffset = 0;
                uint32_t errorCount = 0;

                for (uint16_t i = 0; i < writeCountTotal; i++) {
                    if (f_write(&sd.file, (data + pointerOffset), writeByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                    pointerOffset += 512;
                }

                if (remainderByteSize) {
                    if (f_write(&sd.file, (data + pointerOffset), remainderByteSize, &sd.byteswritten) != FR_OK)
                        errorCount += 1;
                }

                if (errorCount == 0) {
                    if (f_write(&sd.file, eof, strlen(eof), &sd.byteswritten) == FR_OK) {
                        result = SD_OK;
                    }
                }
            }
        }
        f_close(&sd.file);
    }
    // enable keyboard
    keyboard_enable();
    return result;
}

SdResult Controller::sd_checkWavetablesInUse() {
    SdResult result;
    uint8_t missingWavetable = 0;
    bool wavetableMissing[kOscLibrarySize] = {false};

    // disable keyboard
    keyboard_disable();
    // check wavetables
    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
        if (osc[i].wavetableLoaded != -1) {
            if ((f_open(&sd.file, "System/Data/Wavetable.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, 25) == FR_OK)) {
                char searchName[kFileNameSize + 1];
                bool searchActive = true;
                uint16_t searchCounter = 0;

                while (searchActive) {
                    memset(searchName, 0x00, kFileNameSize);
                    f_read(&sd.file, searchName, kFileNameSize, &sd.bytesread);
                    if (strcmp(osc[i].wavetableLoadedData.nameLongR, searchName) == 0) {
                        osc[i].wavetableLoaded = searchCounter;
                        wavetableMissing[i] = false;
                        searchActive = false;
                    } else if (searchCounter >= wavetableLibrarySize) {
                        wavetableMissing[i] = true;
                        searchActive = false;
                    }
                    searchCounter += 1;
                }
            } else {
                wavetableMissing[i] = true;
            }
            f_close(&sd.file);
            if (wavetableMissing[i]) {
                osc_setWavetableSelected(osc[i], -1);
                osc_setWavetableLoaded(osc[i], true);
                osc_setActive(osc[i], false);
            }
        }
    }

    for (uint8_t j = 0; j < kOscLibrarySize; j++) {
        if (wavetableMissing[j])
            missingWavetable += 1;
    }

    if (missingWavetable) {
        (missingWavetable == 1) ? alertType = ALERT_MISSINGWAVETABLE : alertType = ALERT_MISSINGWAVETABLES;
        lcd_drawAlert();
        HAL_Delay(1000);
        lcd_clearAlert();
        result = SD_ERROR;
    } else {
        result = SD_OK;
    }

    f_open(&sd.file, "System/Data/Wavetable.lib", FA_READ);
    f_close(&sd.file);

    // enable keyboard
    keyboard_enable();
    return result;
}

FRESULT Controller::sd_createDirectory(const char *path) {
    FRESULT res;
    res = f_mkdir(path);
    return res;
}

FRESULT Controller::sd_deleteDirectory(const char *path) {
    FRESULT result;
    DIR dir;
    FILINFO fileInfo;
    char file[64] = "";
    bool listFile = true;

    result = f_opendir(&dir, path);
    if (result)
        return result;

    while (listFile) {
        result = f_readdir(&dir, &fileInfo);
        if ((result == FR_OK) && (fileInfo.fname[0] != 0)) {
            memset(file, 0x00, strlen(file));
            sprintf((char *)file, "%s/%s", path, fileInfo.fname);
            (fileInfo.fattrib & AM_DIR) ? sd_deleteDirectory(file) : f_unlink(file);
        } else {
            listFile = false;
        }
    }

    f_closedir(&dir);
    f_unlink(path);
    return result;
}

/* Sdram functions -----------------------------------------------------------*/

void Controller::sdram_write16BitAudio(uint32_t ramAddress_, int16_t data_) {
    *(__IO int16_t *)(ramAddress_) = data_;
}

int16_t Controller::sdram_read16BitAudio(uint32_t ramAddress_) {
    return (int16_t)(*(__IO int16_t *)(ramAddress_));
}

void Controller::sdram_write24BitAudio(uint32_t ramAddress_, int32_t data_) {
    uint8_t a = (uint8_t)((data_) & 0xFF);
    uint8_t b = (uint8_t)((data_ >> 8) & 0xFF);
    uint8_t c = (uint8_t)((data_ >> 16) & 0xFF);

    *(__IO uint8_t *)(ramAddress_) = a;
    *(__IO uint8_t *)(ramAddress_ + 1) = b;
    *(__IO uint8_t *)(ramAddress_ + 2) = c;
}

int32_t Controller::sdram_read24BitAudio(uint32_t ramAddress_) {
    volatile uint8_t *ptrA = (volatile uint8_t *)(ramAddress_);
    volatile uint8_t *ptrB = (volatile uint8_t *)(ramAddress_ + 1);
    volatile uint8_t *ptrC = (volatile uint8_t *)(ramAddress_ + 2);
    uint8_t d;
    (*ptrC >> 7) ? d = 0xFF : d = 0x00;

    int32_t audioData = (int32_t)((d << 24) | ((*ptrC) << 16) | ((*ptrB) << 8) | (*ptrA));
    return audioData;
}

void Controller::sdram_fadeOut24BitAudio(uint32_t ramAddress_, uint32_t sampleSize_, uint16_t fadeOutSize_) {
    uint32_t address = ramAddress_ + (3 * (sampleSize_ - fadeOutSize_));
    float decrement = 1.0f / fadeOutSize_;
    float multiplier = 1.0f;
    for (uint16_t i = 0; i < fadeOutSize_; i++) {
        uint32_t sampleAddress = address + (3 * i);
        int32_t input = sdram_read24BitAudio(sampleAddress);
        int32_t output = (int32_t)(input * multiplier);
        sdram_write24BitAudio(sampleAddress, output);
        multiplier -= decrement;
    }
}

/* Lcd functions -------------------------------------------------------------*/

void Controller::lcd_initialize() {
    lcd.initialize();
}

void Controller::lcd_test() {
    lcd.setForeColor(GRAY_75);
    lcd.setBackColor(BLACK);

    uint16_t boxSize = 26;

    for (uint8_t i = 0; i < 33; i++) {
        uint16_t x = 9 + (i * boxSize);
        lcd.drawVLine(x, 6, 468);
    }

    for (uint8_t i = 0; i < 19; i++) {
        uint16_t y = 6 + (i * boxSize);
        lcd.drawHLine(9, y, 833);
    }

    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);

    for (uint8_t i = 4; i < 29; i++) {
        uint16_t x = 9 + (i * boxSize);
        lcd.drawVLine(x, 6 + (3 * 26), 312);
    }

    for (uint8_t i = 3; i < 16; i++) {
        uint16_t y = 6 + (i * boxSize);
        lcd.drawHLine(9 + (4 * 26), y, 624);
    }

    uint16_t xPos = 9;
    uint16_t yPos = 6;

    lcd.setForeColor(BLACK);
    lcd.fillRect(xPos + (0 * 26) + 1, yPos + (0 * 26) + 1, (boxSize * 3) - 1,
                 (boxSize * 3) - 1);
    lcd.fillRect(xPos + (0 * 26) + 1, yPos + (15 * 26) + 1, (boxSize * 3) - 1,
                 (boxSize * 3) - 1);
    lcd.fillRect(xPos + (29 * 26) + 1, yPos + (0 * 26) + 1, (boxSize * 3) - 1,
                 (boxSize * 3) - 1);
    lcd.fillRect(xPos + (29 * 26) + 1, yPos + (15 * 26) + 1, (boxSize * 3) - 1,
                 (boxSize * 3) - 1);

    RGB16Color cArray01[] = {BLACK, RED, PINK, BLUE, CYAN, GREEN, YELLOW, WHITE};

    for (uint8_t i = 0; i < 8; i++) {
        lcd.setForeColor(cArray01[i]);
        lcd.fillRect(xPos + (4 * 26) + (i * 3 * 26) + 1, yPos + (3 * 26) + 1,
                     (boxSize * 3) - 1, (boxSize * 3) - 1);
    }

    RGB16Color cArray02[] = {0x10A2, 0x18E3, 0x2945, 0x3186, 0x39E7, 0x4228,
                             0x528A, 0x5ACB, 0x632C, 0x6B6D, 0x7BCF, 0x8410,
                             0x8C71, 0x94B2, 0xA514, 0xAD55, 0xB5B6, 0xBDF7,
                             0xCE59, 0xD69A, 0xDEFB, 0xE73C, 0xF79E, 0xFFFF};

    for (uint8_t i = 0; i < 24; i++) {
        lcd.setForeColor(cArray02[i]);
        lcd.fillRect(xPos + (4 * 26) + (i * 26) + 1, yPos + (6 * 26) + 1, (boxSize * 1) - 1, (boxSize * 2) - 1);
    }

    lcd.setForeColor(BLACK);
    lcd.fillRect(xPos + (4 * 26) + (9 * 26) + 1, yPos + (8 * 26) + 1, (boxSize * 6) - 1, (boxSize * 2) - 1);
    lcd.setForeColor(BLACK);
    lcd.fillRect(xPos + (4 * 26) + (15 * 26) + 1, yPos + (8 * 26) + 1, (boxSize * 9) - 1, (boxSize * 2) - 1);

    for (uint8_t i = 0; i < 59; i++) {
        lcd.setForeColor(WHITE);
        lcd.fillRect(xPos + (4 * 26) + (i * 4), yPos + (8 * 26) + 1, 2, (boxSize * 2) - 1);
        lcd.setForeColor(BLACK);
        lcd.fillRect(xPos + (4 * 26) + (i * 4) + 2, yPos + (8 * 26) + 1, 2, (boxSize * 2) - 1);
    }

    for (uint8_t i = 0; i < 5; i++) {
        lcd.setForeColor(WHITE);
        lcd.fillRect(xPos + (4 * 26) + (15 * 26) + (i * 52), yPos + (8 * 26) + 1, 26, (boxSize * 2) - 1);
    }

    lcd.setAlignment(CENTER);
    lcd.setForeColor(WHITE);
    lcd.setFont(FONT_05x07);
    lcd.drawText("RANDOMWAVES", 11, 427, 236);

    lcd.setForeColor(WHITE);
    lcd.fillRect(xPos + (4 * 26) + 1, yPos + (10 * 26) + 1, (boxSize * 24) - 1, (boxSize * 2) - 1);

    RGB16Color redArray[] = {0xFD34, 0xFCB2, 0xFC30, 0xFBCF, 0xFB4D, 0xFACB,
                             0xFA49, 0xF9E7, 0xF965, 0xF8E3, 0xF861, 0xF800,
                             0xF800, 0xE000, 0xD000, 0xC800, 0xB800, 0xA800,
                             0x9800, 0x8800, 0x7800, 0x6800, 0x5800, 0x4000};

    RGB16Color greenArray[] = {0xAFF5, 0x97F2, 0x87F0, 0x7FEF, 0x6FED, 0x5FEB,
                               0x4FE9, 0x3FE7, 0x2FE5, 0x1FE3, 0x0FE1, 0x07E0,
                               0x07E0, 0x0780, 0x0700, 0x0680, 0x0600, 0x05A0,
                               0x0520, 0x04A0, 0x0420, 0x03C0, 0x0340, 0x02C0};

    RGB16Color blueArray[] = {0xA53F, 0x94BF, 0x843F, 0x7BDF, 0x6B5F, 0x5ADF,
                              0x4A5F, 0x39FF, 0x297F, 0x18FF, 0x087F, 0x001F,
                              0x001F, 0x001E, 0x001C, 0x001A, 0x0018, 0x0016,
                              0x0014, 0x0012, 0x0010, 0x000F, 0x000D, 0x000B};

    for (uint8_t i = 0; i < 24; i++) {
        lcd.setForeColor(redArray[i]);
        lcd.fillRect(xPos + (4 * 26) + (i * 26) + 1, yPos + (12 * 26) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
        lcd.setForeColor(greenArray[i]);
        lcd.fillRect(xPos + (4 * 26) + (i * 26) + 1, yPos + (13 * 26) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
        lcd.setForeColor(blueArray[i]);
        lcd.fillRect(xPos + (4 * 26) + (i * 26) + 1, yPos + (14 * 26) + 1, (boxSize * 1) - 1, (boxSize * 1) - 1);
    }

    lcd.setFont(FONT_10x14);
    lcd.setAlignment(CENTER);
    lcd.setForeColor(GRAY_75);
    lcd.drawText("01", 2, xPos + 41, yPos + 33);
    lcd.drawText("02", 2, xPos + 795, yPos + 33);
    lcd.drawText("03", 2, xPos + 41, yPos + 423);
    lcd.drawText("04", 2, xPos + 795, yPos + 423);
}

void Controller::lcd_update() {
    lcd_drawPlay();
    lcd_drawIcon();
    lcd_drawText();
    lcd_drawLimitAlert();
    lcd_drawBankShift();
    lcd_drawCountDown();
    lcd_drawTransition();
}

void Controller::lcd_drawLogo() {
    if (animation) {
        const RGB16Color *indexPtr = (const RGB16Color *)(RAM_IMAGE_LOGO_PALETTE_ADDRESS);
        const uint8_t *dataPtr = (const uint8_t *)(RAM_IMAGE_LOGO_DATA_ADDRESS);
        lcd.fadeRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight, true, 40, 25);
        HAL_Delay(1000);
        lcd.fadeRGB16Image(indexPtr, dataPtr, kImageLogoPalette, kImageLogoX, kImageLogoY, kImageLogoWidth, kImageLogoHeight, false, 40, 25);
        HAL_Delay(500);
        lcd.setForeColor(WHITE);
        lcd.setBackColor(BLACK);
        lcd.clearScreen();
    }
}

void Controller::lcd_drawPage() {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    uint8_t step = 20;
    uint8_t delay = 15;

    palettePtr = (const RGB16Color *)(RAM_IMAGE_MENU_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_MENU_DATA_ADDRESS);

    (animation)
        ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageMenuX, kImageMenuY, kImageMenuWidth, kImageMenuHeight, true, step, delay)
        : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageMenuX, kImageMenuY, kImageMenuWidth, kImageMenuHeight);

    palettePtr = (const RGB16Color *)(RAM_IMAGE_KEY_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_KEY_DATA_ADDRESS);

    (animation)
        ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageKeyX, kImageKeyY, kImageKeyWidth, kImageKeyHeight, true, step, delay)
        : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageKeyX, kImageKeyY, kImageKeyWidth, kImageKeyHeight);

    palettePtr = (const RGB16Color *)(RAM_IMAGE_OSC_A_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_OSC_A_DATA_ADDRESS);

    (animation)
        ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageOscAX, kImageOscAY, kImageOscAWidth, kImageOscAHeight, true, step, delay)
        : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageOscAX, kImageOscAY, kImageOscAWidth, kImageOscAHeight);

    palettePtr = (const RGB16Color *)(RAM_IMAGE_OSC_B_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_OSC_B_DATA_ADDRESS);

    (animation)
        ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageOscBX, kImageOscBY, kImageOscBWidth, kImageOscBHeight, true, step, delay)
        : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageOscBX, kImageOscBY, kImageOscBWidth, kImageOscBHeight);

    palettePtr = (const RGB16Color *)(RAM_IMAGE_FILTER_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_FILTER_DATA_ADDRESS);

    (animation) ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageFilterX, kImageFilterY, kImageFilterWidth, kImageFilterHeight, true, step, delay)
                : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageFilterX, kImageFilterY, kImageFilterWidth, kImageFilterHeight);

    palettePtr = (const RGB16Color *)(RAM_IMAGE_ENV_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_IMAGE_ENV_DATA_ADDRESS);

    (animation)
        ? lcd.fadeRGB16Image(palettePtr, dataPtr, 64, kImageEnvX, kImageEnvY, kImageEnvWidth, kImageEnvHeight, true, step, delay)
        : lcd.drawRGB16Image(palettePtr, dataPtr, 64, kImageEnvX, kImageEnvY, kImageEnvWidth, kImageEnvHeight);
}

// alert functions

void Controller::lcd_drawAlert() {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    indexPtr = (const RGB16Color *)(RAM_ICON_ALERT_PALETTE_ADDRESS);

    dataPtr = (const uint8_t *)(RAM_ICON_ALERT_L_DATA_ADDRESS);
    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconAlertX[0], kIconAlertY, kIconAlertWidth, kIconAlertHeight);

    dataPtr = (const uint8_t *)(RAM_ICON_ALERT_R_DATA_ADDRESS);
    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconAlertX[1], kIconAlertY, kIconAlertWidth, kIconAlertHeight);

    lcd.setBackColor(BLACK);
    lcd.setForeColor(WHITE);
    lcd.clearRect(334, 139, 186, 18);
    lcd.drawHLine(334, 139, 186);
    lcd.drawHLine(334, 157, 186);

    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);

    const char *alertText;

    switch (alertType) {
    case ALERT_MEASUREUP:
    case ALERT_MEASUREDOWN:
    case ALERT_BARUP:
    case ALERT_BARDOWN:
    case ALERT_QUANTIZEUP:
    case ALERT_QUANTIZEDOWN:
        alertText = kAlertTextResetPlay;
        break;

    case ALERT_NEWFILE:
        alertText = kAlertTextNewFile;
        break;

    case ALERT_LOADFILE:
        alertText = kAlertTextLoadFile;
        break;

    case ALERT_SAVEFILE:
        alertText = kAlertTextSaveFile;
        break;

    case ALERT_CLEARFILE:
        alertText = kAlertTextClearFile;
        break;

    case ALERT_OVERWRITEFILE:
        alertText = kAlertTextOverwriteFile;
        break;

    case ALERT_NEWSYNTHKIT:
        alertText = kAlertTextNewSynthkit;
        break;

    case ALERT_LOADSYNTHKIT:
        alertText = kAlertTextLoadSynthkit;
        break;

    case ALERT_SAVESYNTHKIT:
        alertText = kAlertTextSaveSynthkit;
        break;

    case ALERT_CLEARSYNTHKIT:
        alertText = kAlertTextClearSynthkit;
        break;

    case ALERT_OVERWRITESYNTHKIT:
        alertText = kAlertTextOverwriteSynthkit;
        break;

    case ALERT_LOADSUCCESS:
        alertText = kAlertTextLoadSuccess;
        break;

    case ALERT_LOADERROR:
        alertText = kAlertTextLoadError;
        break;

    case ALERT_SAVESUCCESS:
        alertText = kAlertTextSaveSuccess;
        break;

    case ALERT_SAVEERROR:
        alertText = kAlertTextSaveError;
        break;

    case ALERT_CLEARSUCCESS:
        alertText = kAlertTextClearSuccess;
        break;

    case ALERT_CLEARERROR:
        alertText = kAlertTextClearError;
        break;

    case ALERT_MISSINGWAVETABLE:
        alertText = kAlertTextMissingWavetable;
        break;

    default:
        break;
    }

    lcd.drawText(alertText, strlen(alertText), 427, 144);
}

void Controller::lcd_clearAlert() {
    lcd.setBackColor(BLACK);
    lcd.clearRect(317, 139, 220, 19);
    lcd_drawSong(activeBankNum);
}

// limit alert functions

void Controller::lcd_drawLimitAlert() {
    if (limitAlertShowFlag) {
        limitAlertShowFlag = false;
        stopLimitAlertTimer();
        startLimitAlertTimer();
        if (!limitAlertActive) {
            limitAlertActive = true;
            lcd.setBackColor(BLACK);
            lcd.setFont(FONT_07x09);
            lcd.setAlignment(CENTER);
            lcd.setForeColor(kLayerColorPalette[9]);
            lcd.drawText("!", 1, kLimitAlertX, kLimitAlertY);
        }
    } else if (limitAlertClearFlag) {
        limitAlertClearFlag = false;
        limitAlertActive = false;
        lcd.setFont(FONT_07x09);
        lcd.setAlignment(CENTER);
        lcd.setForeColor(kLayerColorPalette[9]);
        lcd.setBackColor(BLACK);
        lcd.drawText(" ", 1, kLimitAlertX, kLimitAlertY);
    }
}

// sd functions

void Controller::lcd_drawSdDataIntro() {
    lcd.setForeColor(kLayerColorPalette[9]);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(CENTER);
    lcd.drawText("[ ANALYZING SDCARD ]", 20, 427, 460);
}

void Controller::lcd_drawSdData() {
    lcd.setBackColor(BLACK);
    lcd.setForeColor(kLayerColorPalette[9]);
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_05x07);

    char tab[] = "   |   ";
    char dot = '.';
    char textTitle[] = "SYNTHGIRL V";
    char textFree[] = "MB FREE";
    char textFile[] = " FILES";
    char textSynthkit[] = " SYNTHKITS";
    char textLfo[] = " LFOS";
    char textWtable[] = " WTABLES";

    char sdText[110] = {};
    char versionMajor[3];
    char versionMinor[3];
    sprintf(versionMajor, "%01d", kVersionMajor);
    sprintf(versionMinor, "%02d", kVersionMinor);
    strncpy(sdText, textTitle, 11);
    strncat(sdText, versionMajor, 2);
    strncat(sdText, &dot, 1);
    strncat(sdText, versionMinor, 2);
    strcat(sdText, tab);

    if (sd.ready) {
        char numFree[6];
        sprintf(numFree, "%05d", sd.freeSpace);
        strcat(sdText, numFree);
    } else {
        strcat(sdText, "-----");
    }
    strcat(sdText, textFree);
    strcat(sdText, tab);

    if (sd.ready) {
        char numFile[4];
        sprintf(numFile, "%03d", fileLibrarySize);
        strcat(sdText, numFile);
    } else {
        strcat(sdText, "---");
    }
    strcat(sdText, textFile);
    strcat(sdText, tab);

    if (sd.ready) {
        char numSynthkit[4];
        sprintf(numSynthkit, "%03d", synthkitLibrarySize);
        strcat(sdText, numSynthkit);
    } else {
        strcat(sdText, "---");
    }
    strcat(sdText, textSynthkit);
    strcat(sdText, tab);

    if (sd.ready) {
        char numLfo[4];
        sprintf(numLfo, "%03d", kMaxLfoType);
        strcat(sdText, numLfo);
    } else {
        strcat(sdText, "---");
    }
    strcat(sdText, textLfo);
    strcat(sdText, tab);

    if (sd.ready) {
        char numWavetable[4];
        if (wavetableLibrarySize < 10000) {
            sprintf(numWavetable, "%04d", wavetableLibrarySize);
            strcat(sdText, numWavetable);
        } else {
            sprintf(numWavetable, "%04d", wavetableLibrarySize / 1000);
            strcat(sdText, numWavetable);
            strcat(sdText, "K");
        }
    } else {
        strcat(sdText, "----");
    }
    strcat(sdText, textWtable);

    lcd.drawText(sdText, strlen(sdText), 427, 460);
}

void Controller::lcd_clearSdData() {
    lcd.setForeColor(BLACK);
    lcd.fillRect(20, 460, 814, 7);
}

void Controller::lcd_drawSdAlert(SdResult result_) {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    indexPtr = (const RGB16Color *)(RAM_ICON_ALERT_PALETTE_ADDRESS);

    dataPtr = (const uint8_t *)(RAM_ICON_ALERT_L_DATA_ADDRESS);
    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconAlertX[0], kIconAlertY, kIconAlertWidth, kIconAlertHeight);

    dataPtr = (const uint8_t *)(RAM_ICON_ALERT_R_DATA_ADDRESS);
    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconAlertX[1], kIconAlertY, kIconAlertWidth, kIconAlertHeight);

    lcd.setBackColor(BLACK);
    lcd.setForeColor(WHITE);
    lcd.clearRect(334, 139, 186, 18);
    lcd.drawHLine(334, 139, 186);
    lcd.drawHLine(334, 157, 186);

    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);

    const char *alertPtr;

    switch (result_) {
    case SD_ERROR_DETECT:
        alertPtr = kSdAlertTextInsert;
        break;

    case SD_ERROR_MOUNT:
        alertPtr = kSdAlertTextFormat;
        break;

    case SD_ERROR_SERIAL:
        alertPtr = kSdAlertTextSerial;
        break;

    case SD_ERROR_SYSTEMFOLDER:
        alertPtr = kSdAlertTextSystemFolder;
        break;

    case SD_ERROR_WAVETABLEFOLDER:
        alertPtr = kSdAlertTextWavetableFolder;
        break;

    case SD_ERROR_SYSTEMFILE:
        alertPtr = kSdAlertTextSystemFile;
        break;
    }

    lcd.drawText(alertPtr, strlen(alertPtr), 427, 144);
}

void Controller::lcd_clearSdAlert() {
    lcd.setBackColor(BLACK);
    lcd.clearRect(317, 139, 220, 19);
    lcd_drawSong(activeBankNum);
}

void Controller::lcd_drawInitSdAlert(SdResult result_) {
    lcd.drawInitSdAlert(result_);
}

void Controller::lcd_clearInitSdAlert() {
    lcd.clearInitSdAlert();
}

// menu functions

void Controller::lcd_drawMenuIcon(Menu menu_) {
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_10x14);
    lcd.setAlignment(LEFT);
    lcd.setSpacing(4);

    switch (menu_) {
    case MAIN_MENU:
        lcd.clearRect(kMenuIconX, kMenuIconY, 38, 15);
        for (uint8_t i = 0; i < 5; i++) {
            for (uint8_t j = 0; j < 2; j++) {
                lcd.fillRect(kMenuIconX + 1 + (i * 8), kMenuIconY + 1 + (j * 8), 4, 4);
            }
        }
        break;

    case FILE_MENU:
        lcd.drawText("FIL", 3, kMenuIconX, kMenuIconY);
        break;

    case SYNTHKIT_MENU:
        lcd.drawText("SYN", 3, kMenuIconX, kMenuIconY);
        break;

    case SYSTEM_MENU:
        lcd.drawText("MAS", 3, kMenuIconX, kMenuIconY);
        break;

    case RHYTHM_MENU:
        lcd.drawText("RHY", 3, kMenuIconX, kMenuIconY);
        break;

    case METRO_MENU:
        lcd.drawText("MET", 3, kMenuIconX, kMenuIconY);
        break;

    case OSC_A0_MENU:
    case OSC_A1_MENU:
    case OSC_B0_MENU:
    case OSC_B1_MENU:
        lcd.drawText("OSC", 3, kMenuIconX, kMenuIconY);
        break;

    case OSC_A2_MENU:
    case OSC_A3_MENU:
    case OSC_B2_MENU:
    case OSC_B3_MENU:
        lcd.drawText("LFO", 3, kMenuIconX, kMenuIconY);
        break;

    case EQ_MENU:
        lcd.drawText("PEQ", 3, kMenuIconX, kMenuIconY);
        break;

    case FILTER_0_MENU:
    case FILTER_1_MENU:
        lcd.drawText("FIL", 3, kMenuIconX, kMenuIconY);
        break;

    case ENVELOPE_MENU:
        lcd.drawText("ENV", 3, kMenuIconX, kMenuIconY);
        break;

    case EFFECT_0_MENU:
    case EFFECT_1_MENU:
        lcd.drawText("EFF", 3, kMenuIconX, kMenuIconY);
        break;

    case REVERB_MENU:
        lcd.drawText("REV", 3, kMenuIconX, kMenuIconY);
        break;

    case KEY_MENU:
        lcd.drawText("KEY", 3, kMenuIconX, kMenuIconY);
        break;

    case SONG_MENU:
        lcd.drawText("SON", 3, kMenuIconX, kMenuIconY);
        break;
    }
}

void Controller::lcd_drawFileBox(uint8_t menuTab_, FileStatus status_) {
    uint16_t xPos = kMenuBoxX[menuTab_];
    uint16_t yPos = kMenuBoxY;

    switch (status_) {
    case FILE_NONE:
        lcd.setForeColor(MAGENTA);
        lcd.setBackColor(BLACK);
        lcd.clearRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        lcd.drawRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        break;

    case FILE_MISSING:
        lcd.setForeColor(MAGENTA);
        lcd.setBackColor(BLACK);
        lcd.clearRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        lcd.drawRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        break;

    case FILE_INCOMPATIBLE:
        lcd.setForeColor(MAGENTA);
        lcd.setBackColor(BLACK);
        lcd.clearRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        lcd.fillRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        break;

    case FILE_INACTIVE:
        lcd.setForeColor(WHITE);
        lcd.setBackColor(BLACK);
        lcd.clearRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        lcd.drawRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        break;

    case FILE_ACTIVE:
        lcd.setForeColor(WHITE);
        lcd.setBackColor(BLACK);
        lcd.fillRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
        break;
    }
}

void Controller::lcd_clearFileBox(uint8_t menuTab_) {
    uint16_t xPos = kMenuBoxX[menuTab_];
    uint16_t yPos = kMenuBoxY;
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);
    lcd.clearRect(xPos, yPos, kMenuBoxWidth, kMenuBoxHeight);
}

void Controller::lcd_transitionMenu() {
    uint8_t preMenuType;
    uint8_t menuType;

    switch (preMenu) {
    case MAIN_MENU:
        preMenuType = 0;
        break;

    case FILE_MENU:
    case SYNTHKIT_MENU:
    case RHYTHM_MENU:
    case METRO_MENU:
        preMenuType = 1;
        break;

    case OSC_A0_MENU:
    case OSC_A1_MENU:
    case OSC_A2_MENU:
    case OSC_A3_MENU:
    case OSC_B0_MENU:
    case OSC_B1_MENU:
    case OSC_B2_MENU:
    case OSC_B3_MENU:
        preMenuType = 2;
        break;

    case SYSTEM_MENU:
    case EQ_MENU:
    case ENVELOPE_MENU:
    case FILTER_0_MENU:
    case FILTER_1_MENU:
    case EFFECT_0_MENU:
    case EFFECT_1_MENU:
    case REVERB_MENU:
    case KEY_MENU:
        preMenuType = 3;
        break;

    case SONG_MENU:
        preMenuType = 4;
        break;
    }

    switch (menu) {
    case MAIN_MENU:
        menuType = 0;
        break;

    case FILE_MENU:
    case SYNTHKIT_MENU:
    case RHYTHM_MENU:
    case METRO_MENU:
        menuType = 1;
        break;

    case OSC_A0_MENU:
    case OSC_A1_MENU:
    case OSC_A2_MENU:
    case OSC_A3_MENU:
    case OSC_B0_MENU:
    case OSC_B1_MENU:
    case OSC_B2_MENU:
    case OSC_B3_MENU:
        menuType = 2;
        break;

    case SYSTEM_MENU:
    case EQ_MENU:
    case ENVELOPE_MENU:
    case FILTER_0_MENU:
    case FILTER_1_MENU:
    case EFFECT_0_MENU:
    case EFFECT_1_MENU:
    case REVERB_MENU:
    case KEY_MENU:
        menuType = 3;
        break;

    case SONG_MENU:
        menuType = 4;
        break;
    }

    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);

    switch (preMenuType) {
    case 0: // main
        lcd.clearRect(kMenuLine4X[0] + 1, 29, 161, 61);
        lcd.clearRect(kMenuLine4X[1] + 1, 29, 161, 61);
        lcd.clearRect(kMenuLine4X[2] + 1, 29, 161, 61);
        lcd.clearRect(kMenuLine4X[3] + 1, 29, 161, 61);
        break;

    case 1: // file / synthkit / rhythm / metro
        if (menuType != preMenuType) {
        }

        switch (preMenu) {
        case FILE_MENU:
            fileStatus = FILE_NONE;
            for (uint8_t i = 1; i < 4; i++) {
                lcd_clearFileBox(i);
            }
            break;

        case SYNTHKIT_MENU:
            synthkitStatus = FILE_NONE;
            for (uint8_t i = 1; i < 4; i++) {
                lcd_clearFileBox(i);
            }
            break;

        case RHYTHM_MENU:
            lcd.setBackColor(BLACK);
            lcd.clearRect(kMenuData4X[0] - 42, kMenuHeaderY + 1, 42, 7);
            lcd.clearRect(kMenuData4X[1] - 10, kMenuHeaderY, 10, 10);
            lcd.clearRect(kMenuData4X[2] - 10, kMenuHeaderY, 10, 10);
            lcd.clearRect(kMenuData4X[3] - 10, kMenuHeaderY, 10, 10);

            rhythm.measureLock = true;
            rhythm.barLock = true;
            rhythm.quantizeLock = true;
            break;

        default:
            break;
        }

        break;

    case 2: // oscA / oscB
        if (menuType != preMenuType) {
            lcd.setBackColor(BLACK);
            lcd.clearRect(kMenuLine4X[2], kMenuLineY, 162, 60);
            lcd.clearRect(kMenuLine4X[3], kMenuLineY, 162, 60);
        }

        if (((preMenu == OSC_A0_MENU) || (preMenu == OSC_A1_MENU) || (preMenu == OSC_B0_MENU) || (preMenu == OSC_B1_MENU)) &&
            ((menu != OSC_A0_MENU) && (menu != OSC_A1_MENU) && (menu != OSC_B0_MENU) && (menu != OSC_B1_MENU))) {
            lcd.clearRect(kMenuData4X[1] - 60, kMenuHeaderY, 60, 30);
        }

        switch (preMenu) {
        case OSC_A0_MENU:
            if (menu != OSC_A1_MENU)
                lcd_drawInfo_Osc_Select(osc[0], false);
            break;

        case OSC_A1_MENU:
            lcd_drawInfo_Osc_Select(osc[0], false);
            break;

        case OSC_A2_MENU:
            lcd_drawInfo_OscLfo_Select(osc[0], osc[0].lfo[0], false);
            break;

        case OSC_A3_MENU:
            lcd_drawInfo_OscLfo_Select(osc[0], osc[0].lfo[1], false);
            break;

        case OSC_B0_MENU:
            if (menu != OSC_B1_MENU)
                lcd_drawInfo_Osc_Select(osc[1], false);
            break;

        case OSC_B1_MENU:
            lcd_drawInfo_Osc_Select(osc[1], false);
            break;

        case OSC_B2_MENU:
            lcd_drawInfo_OscLfo_Select(osc[1], osc[1].lfo[0], false);
            break;

        case OSC_B3_MENU:
            lcd_drawInfo_OscLfo_Select(osc[1], osc[1].lfo[1], false);
            break;
        }
        break;

    case 3: // system / eq / envelope / filter / effect / reverb / key
        if (menuType != preMenuType) {
            lcd.clearRect(kMenuLine4X[1] + 1, 29, 161, 61);
            lcd.clearRect(kMenuLine4X[2] + 1, 29, 161, 61);
            lcd.clearRect(kMenuLine4X[3] + 1, 29, 161, 61);
        }

        switch (preMenu) {
        case ENVELOPE_MENU:
            lcd_drawInfo_Envelope_Select(false);
            break;

        case FILTER_0_MENU:
            lcd_drawInfo_Filter_Select(0, false);
            break;

        case FILTER_1_MENU:
            lcd_drawInfo_Filter_Select(1, false);
            break;

        case KEY_MENU:
            lcd_drawInfo_Key_Select(false);
            break;
        }
        break;

    case 4: // song
        lcd_drawInfo_Song_Select(false);
        lcd.setBackColor(BLACK);
        lcd.clearRect(kMenuLine4X[1] + 1, 29, 487, 61);
        lcd.clearRect(kMenuData4X[0] - 35, kMenuHeaderY, 35, 9);
        break;
    }

    switch (menuType) {
    case 0: // main
        lcd.clearRect(182 + 1, 29, 650, 61);
        for (uint8_t i = 1; i < 8; i++) {
            lcd.drawVLine(kMenuLine8X[i], kMenuLineY, kMenuLineHeight);
        }
        break;

    case 1: // file / synthkit / rhythm / metro
        for (uint8_t i = 1; i < 4; i++) {
            lcd.drawVLine(kMenuLine4X[i], kMenuLineY, kMenuLineHeight);
        }
        break;

    case 2: // oscA / oscB
        if (menuType != preMenuType) {
            lcd.clearRect(kMenuLine4X[2], kMenuHeaderY, 324, 54);
            lcd.drawVLine(kMenuLine4X[1], kMenuLineY, kMenuLineHeight);
            for (int i = 3; i < 7; i++) {
                lcd.drawVLine(kMenuLine8X[i + 1], kMenuLineY, kMenuLineHeight);
            }
        }

        switch (menu) {
        case OSC_A0_MENU:
            lcd_drawInfo_Osc_Select(osc[0], 1);
            break;

        case OSC_A1_MENU:
            lcd_drawInfo_Osc_Select(osc[0], 2);
            break;

        case OSC_A2_MENU:
            lcd_drawInfo_OscLfo_Select(osc[0], osc[0].lfo[0], true);
            break;

        case OSC_A3_MENU:
            lcd_drawInfo_OscLfo_Select(osc[0], osc[0].lfo[1], true);
            break;

        case OSC_B0_MENU:
            lcd_drawInfo_Osc_Select(osc[1], 1);
            break;

        case OSC_B1_MENU:
            lcd_drawInfo_Osc_Select(osc[1], 2);
            break;

        case OSC_B2_MENU:
            lcd_drawInfo_OscLfo_Select(osc[1], osc[1].lfo[0], true);
            break;

        case OSC_B3_MENU:
            lcd_drawInfo_OscLfo_Select(osc[1], osc[1].lfo[1], true);
            break;
        }
        break;

    case 3: // system / eq / envelope / filter / effect / reverb / key
        if ((menuType != preMenuType) || (preMenu == EQ_MENU) || (menu == EQ_MENU)) {
            lcd.clearRect(kMenuLine4X[1] + 1, 29, 161, 61);
            lcd.clearRect(kMenuLine4X[2] + 1, 29, 161, 61);
            lcd.clearRect(kMenuLine4X[3] + 1, 29, 161, 61);
        }

        for (int i = 2; i < 8; i++) {
            lcd.drawVLine(kMenuLine8X[i], kMenuLineY, kMenuLineHeight);
        }

        switch (menu) {
        case EQ_MENU:
            break;

        case ENVELOPE_MENU:
            lcd_drawInfo_Envelope_Select(true);
            break;

        case FILTER_0_MENU:
            lcd_drawInfo_Filter_Select(0, true);
            break;

        case FILTER_1_MENU:
            lcd_drawInfo_Filter_Select(1, true);
            break;

        case KEY_MENU:
            lcd_drawInfo_Key_Select(true);
            break;
        }
        break;

    case 4: // song
        lcd.clearRect(kMenuLine4X[1] + 1, 29, 487, 61);
        lcd_drawInfo_Song_Select(true);
        break;
    }
}

void Controller::lcd_transitionSelect() {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    indexPtr = (const RGB16Color *)(RAM_ICON_SELECT_PALETTE_ADDRESS);

    // clear premenutab selection
    if (preMenuTab != -1) {
        uint8_t refMenu;
        if (preMenu == menu) {
            refMenu = menu;
        } else {
            refMenu = preMenu;
            preMenu = menu;
        }
        dataPtr = (const uint8_t *)(RAM_ICON_SELECT_OFF_DATA_ADDRESS);
        switch (refMenu) {
        case EQ_MENU:
            if (preMenuTab == 0) {
                lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect8X[preMenuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            } else {
                lcd.setForeColor(WHITE);
                lcd.setBackColor(BLACK);
                lcd.setAlignment(LEFT);
                lcd.setFont(FONT_05x07);
                lcd.drawText(" ", 1, kMenuHeader8X[preMenuTab] + 24, 52);
                lcd.drawText(" ", 1, kMenuHeader8X[preMenuTab] + 24, 66);
                lcd.drawText(" ", 1, kMenuHeader8X[preMenuTab] + 24, 80);
            }
            break;

        case SYSTEM_MENU:
        case OSC_A0_MENU:
        case OSC_A1_MENU:
        case OSC_A2_MENU:
        case OSC_A3_MENU:
        case OSC_B0_MENU:
        case OSC_B1_MENU:
        case OSC_B2_MENU:
        case OSC_B3_MENU:
        case FILTER_0_MENU:
        case FILTER_1_MENU:
        case ENVELOPE_MENU:
        case EFFECT_0_MENU:
        case EFFECT_1_MENU:
        case REVERB_MENU:
        case KEY_MENU:
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect8X[preMenuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            break;

        default:
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect4X[preMenuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            break;
        }
    }

    // draw menutab selection
    if (menuTab != -1) {
        dataPtr = (const uint8_t *)(RAM_ICON_SELECT_ON_DATA_ADDRESS);
        switch (menu) {
        case EQ_MENU:
            if (menuTab == 0) {
                lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect8X[menuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            } else {
                lcd.setForeColor(WHITE);
                lcd.setBackColor(BLACK);
                lcd.setAlignment(LEFT);
                lcd.setFont(FONT_05x07);
                (subMenuTab == 0) ? lcd.drawText(">", 1, kMenuHeader8X[menuTab] + 24, 80) : lcd.drawText(" ", 1, kMenuHeader8X[menuTab] + 24, 80);
                (subMenuTab == 1) ? lcd.drawText(">", 1, kMenuHeader8X[menuTab] + 24, 66) : lcd.drawText(" ", 1, kMenuHeader8X[menuTab] + 24, 66);
                (subMenuTab == 2) ? lcd.drawText(">", 1, kMenuHeader8X[menuTab] + 24, 52) : lcd.drawText(" ", 1, kMenuHeader8X[menuTab] + 24, 52);
            }
            break;

        case SYSTEM_MENU:
        case OSC_A0_MENU:
        case OSC_A1_MENU:
        case OSC_A2_MENU:
        case OSC_A3_MENU:
        case OSC_B0_MENU:
        case OSC_B1_MENU:
        case OSC_B2_MENU:
        case OSC_B3_MENU:
        case FILTER_0_MENU:
        case FILTER_1_MENU:
        case ENVELOPE_MENU:
        case EFFECT_0_MENU:
        case EFFECT_1_MENU:
        case REVERB_MENU:
        case KEY_MENU:
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect8X[menuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            break;

        default:
            lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconSelect4X[menuTab], kIconSelectY, kIconSelectWidth, kIconSelectHeight);
            break;
        }
    }
}

void Controller::lcd_setMenuHeaderState(RGB16Color color_) {
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
}

void Controller::lcd_setMenuDataState(RGB16Color color_) {
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMenuNumState(RGB16Color color_) {
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMenuSignState(RGB16Color color_, LcdFont font_) {
    lcd.setForeColor(color_);
    lcd.setBackColor(BLACK);
    lcd.setFont(font_);
    lcd.setAlignment(RIGHT);
}

void Controller::lcd_setMainMenuHeaderState() {
    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(CENTER);
}

void Controller::lcd_setSongDataState() {
    lcd.setForeColor(BLACK);
    lcd.setBackColor(kLayerColorPalette[0]);
    lcd.setFont(FONT_07x09);
    lcd.setAlignment(LEFT);
}

// main menu functions

void Controller::lcd_drawMainMenu() {
    lcd_drawMenuIcon(MAIN_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMainMenuHeaderState();
    for (uint8_t i = 1; i < 8; i++) {
        lcd.drawVLine(kMenuLine8X[i], kMenuLineY, kMenuLineHeight);
    }

    char title[8][4] = {"TEM", "MEA", "BAR", "QUA", "FL1", "FL2", "EF1", "EF2"};

    for (uint8_t i = 0; i < 8; i++) {
        lcd.drawText(title[i], 3, kMainMenuX[i], kMainMenuHeaderY);
    }

    lcd_drawMain_TempoData();
    lcd_drawMain_MeasureData();
    lcd_drawMain_BarData();
    lcd_drawMain_QuantizeData();
    lcd_drawMain_Filter0Data();
    lcd_drawMain_Filter1Data();
    lcd_drawMain_Effect0Data();
    lcd_drawMain_Effect1Data();
}

void Controller::lcd_drawMain_TempoData() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[0]);
    char kTempoData[3];
    sprintf(kTempoData, "%03d", rhythm.tempo);
    lcd.drawText(kTempoData, 3, kMainMenuX[0], kMainMenuDataY);
}

void Controller::lcd_drawMain_MeasureData() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[1]);
    char kMeasureData[2];
    sprintf(kMeasureData, "%02d", rhythm.measure);
    lcd.drawText(kMeasureData, 2, kMainMenuX[1], kMainMenuDataY);
}

void Controller::lcd_drawMain_BarData() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[2]);
    char kBarData[2];
    sprintf(kBarData, "%02d", rhythm.bar);
    lcd.drawText(kBarData, 2, kMainMenuX[2], kMainMenuDataY);
}

void Controller::lcd_drawMain_QuantizeData() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[3]);
    lcd.drawText(kQuantizeDataLibrary[rhythm.quantize].nameShortR, 2, kMainMenuX[3], kMainMenuDataY);
}

void Controller::lcd_drawMain_Filter0Data() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[4]);
    (filter[0].active) ? lcd.drawText(kFilterTypeDataLibrary[filter[0].type].nameShortL, 3, kMainMenuX[4], kMainMenuDataY) : lcd.drawText("OFF", 3, kMainMenuX[4], kMainMenuDataY);
}

void Controller::lcd_drawMain_Filter1Data() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[5]);
    (filter[1].active) ? lcd.drawText(kFilterTypeDataLibrary[filter[1].type].nameShortL, 3, kMainMenuX[5], kMainMenuDataY) : lcd.drawText("OFF", 3, kMainMenuX[5], kMainMenuDataY);
}

void Controller::lcd_drawMain_Effect0Data() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[7]);
    (effect[0].active) ? lcd.drawText(kEffectTypeDataLibrary[effect[0].type].nameShortL, 3, kMainMenuX[6], kMainMenuDataY) : lcd.drawText("OFF", 3, kMainMenuX[6], kMainMenuDataY);
}

void Controller::lcd_drawMain_Effect1Data() {
    lcd.setAlignment(CENTER);
    lcd.setFont(FONT_07x09);
    lcd.setForeColor(kLayerColorPalette[8]);
    (effect[1].active) ? lcd.drawText(kEffectTypeDataLibrary[effect[1].type].nameShortL, 3, kMainMenuX[7], kMainMenuDataY) : lcd.drawText("OFF", 3, kMainMenuX[7], kMainMenuDataY);
}

// file menu functions

void Controller::lcd_drawFileMenu() {
    lcd_drawMenuIcon(FILE_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderNew, kMenuHeaderTextSize, kMenuHeader4X[0], kMenuHeaderY);
    lcd.drawText(kHeaderLoad, kMenuHeaderTextSize, kMenuHeader4X[1], kMenuHeaderY);
    lcd.drawText(kHeaderSave, kMenuHeaderTextSize, kMenuHeader4X[2], kMenuHeaderY);
    lcd.drawText(kHeaderClear, kMenuHeaderTextSize, kMenuHeader4X[3], kMenuHeaderY);

    lcd_drawFile_NewData();
    lcd_drawFile_LoadData();
    lcd_drawFile_SaveData();
    lcd_drawFile_ClearData();
}

void Controller::lcd_drawFile_NewData() {
    lcd_setMenuDataState(WHITE);
    const char *text;
    (menuTab == 0) ? text = kDataSelect : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawFile_LoadData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char file[11] = "  FILE_";
    char num[4];
    sprintf(num, "%03d", fileMenuCounter + 1);
    strncat(file, num, 3);
    (menuTab == 1) ? text = file : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);
}

void Controller::lcd_drawFile_SaveData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char file[11] = "  FILE_";
    char num[4];
    sprintf(num, "%03d", fileMenuCounter + 1);
    strncat(file, num, 3);
    (menuTab == 2) ? text = file : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[2], kMenuDataY);
}

void Controller::lcd_drawFile_ClearData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char file[11] = "  FILE_";
    char num[4];
    sprintf(num, "%03d", fileMenuCounter + 1);
    strncat(file, num, 3);
    (menuTab == 3) ? text = file : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[3], kMenuDataY);
}

// synthkit menu functions

void Controller::lcd_drawSynthkitMenu() {
    lcd_drawMenuIcon(SYNTHKIT_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderNew, kMenuHeaderTextSize, kMenuHeader4X[0], kMenuHeaderY);
    lcd.drawText(kHeaderLoad, kMenuHeaderTextSize, kMenuHeader4X[1], kMenuHeaderY);
    lcd.drawText(kHeaderSave, kMenuHeaderTextSize, kMenuHeader4X[2], kMenuHeaderY);
    lcd.drawText(kHeaderClear, kMenuHeaderTextSize, kMenuHeader4X[3], kMenuHeaderY);

    lcd_drawSynthkit_NewData();
    lcd_drawSynthkit_LoadData();
    lcd_drawSynthkit_SaveData();
    lcd_drawSynthkit_ClearData();
}

void Controller::lcd_drawSynthkit_NewData() {
    lcd_setMenuDataState(WHITE);
    const char *text;
    (menuTab == 0) ? text = kDataSelect : text = kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawSynthkit_LoadData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char drum[11] = " SYNTH_";
    char num[4];
    sprintf(num, "%03d", synthkitMenuCounter + 1);
    strncat(drum, num, 3);
    (menuTab == 1) ? text = drum : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);
}

void Controller::lcd_drawSynthkit_SaveData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char drum[11] = " SYNTH_";
    char num[4];
    sprintf(num, "%03d", synthkitMenuCounter + 1);
    strncat(drum, num, 3);
    (menuTab == 2) ? text = drum : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[2], kMenuDataY);
}

void Controller::lcd_drawSynthkit_ClearData() {
    lcd_setMenuDataState(WHITE);
    char *text;
    char drum[11] = " SYNTH_";
    char num[4];
    sprintf(num, "%03d", synthkitMenuCounter + 1);
    strncat(drum, num, 3);
    (menuTab == 3) ? text = drum : text = (char *)kDataDashR;
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[3], kMenuDataY);
}

// system menu functions

void Controller::lcd_drawSystemMenu() {
    lcd_drawMenuIcon(SYSTEM_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderVolume, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("PAN ", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd.drawText("LIM ", kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd.drawText("MIDI", kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd.drawText("MIDI", kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd.drawText("SYNC", kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd.drawText("SYNC", kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(" IN", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("OUT", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText(" IN", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("OUT", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawSystem_VolumeData();
    lcd_drawSystem_PanData();
    lcd_drawSystem_LimiterData();
    lcd_drawSystem_MidiInData();
    lcd_drawSystem_MidiOutData();
    lcd_drawSystem_SyncInData();
    lcd_drawSystem_SyncOutData();
}

void Controller::lcd_drawSystem_VolumeData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemVolumeDataLibrary[system.volume].nameLongR, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawSystem_PanData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemPanDataLibrary[system.pan].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawSystem_LimiterData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (system.limiter) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
}

void Controller::lcd_drawSystem_MidiInData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemMidiDataLibrary[system.midiIn].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawSystem_MidiOutData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemMidiDataLibrary[system.midiOut].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
}

void Controller::lcd_drawSystem_SyncInData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemSyncInDataLibrary[system.syncIn].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawSystem_SyncOutData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kSystemSyncOutDataLibrary[system.syncOut].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

// rhythm menu functions

void Controller::lcd_drawRhythmMenu() {
    lcd_drawMenuIcon(RHYTHM_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderTempo, kMenuHeaderTextSize, kMenuHeader4X[0], kMenuHeaderY);
    lcd.drawText(kHeaderMeasure, kMenuHeaderTextSize, kMenuHeader4X[1], kMenuHeaderY);
    lcd.drawText(kHeaderBar, kMenuHeaderTextSize, kMenuHeader4X[2], kMenuHeaderY);
    lcd.drawText(kHeaderQuantize, kMenuHeaderTextSize, kMenuHeader4X[3], kMenuHeaderY);

    lcd_drawRhythm_TempoData();
    lcd_drawRhythm_MeasureData();
    lcd_drawRhythm_BarData();
    lcd_drawRhythm_QuantizeData();
}

void Controller::lcd_drawRhythm_TempoData() {
    lcd_setMenuDataState(WHITE);
    char kData[kMenuDataTextSize];
    sprintf(kData, "       %03d", rhythm.tempo);
    lcd.drawText(kData, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);

    if (system.sync.slaveMode) {
        lcd_setMenuNumState(MAGENTA);
        lcd.drawText(" SLAVE", 6, kMenuData4X[0], kMenuHeaderY + 1);
    } else if (system.sync.masterMode) {
        lcd_setMenuNumState(YELLOW);
        lcd.drawText("MASTER", 6, kMenuData4X[0], kMenuHeaderY + 1);
    }
}

void Controller::lcd_drawRhythm_MeasureData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[rhythm.measure].nameLongR, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);

    lcd_setMenuSignState(WHITE, FONT_07x09);
    (rhythm.measureLock) ? lcd.setForeColor(MAGENTA) : lcd.setForeColor(BLACK);
    lcd.drawText("%", 1, kMenuData4X[1], kMenuHeaderY);
}

void Controller::lcd_drawRhythm_BarData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kNumberDataLibrary[rhythm.bar].nameLongR, kMenuDataTextSize, kMenuData4X[2], kMenuDataY);

    lcd_setMenuSignState(WHITE, FONT_07x09);
    (rhythm.measureLock) ? lcd.setForeColor(MAGENTA) : lcd.setForeColor(BLACK);
    lcd.drawText("%", 1, kMenuData4X[2], kMenuHeaderY);
}

void Controller::lcd_drawRhythm_QuantizeData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kQuantizeDataLibrary[rhythm.quantize].nameLongR, kMenuDataTextSize, kMenuData4X[3], kMenuDataY);

    lcd_setMenuSignState(WHITE, FONT_07x09);
    (rhythm.measureLock) ? lcd.setForeColor(MAGENTA) : lcd.setForeColor(BLACK);
    lcd.drawText("%", 1, kMenuData4X[3], kMenuHeaderY);
}

// metronome menu functions

void Controller::lcd_drawMetroMenu() {
    lcd_drawMenuIcon(METRO_MENU);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderMetronome, kMenuHeaderTextSize, kMenuHeader4X[0], kMenuHeaderY);
    lcd.drawText(kHeaderPrecount, kMenuHeaderTextSize, kMenuHeader4X[1], kMenuHeaderY);
    lcd.drawText(kHeaderSample, kMenuHeaderTextSize, kMenuHeader4X[2], kMenuHeaderY);
    lcd.drawText(kHeaderVolume, kMenuHeaderTextSize, kMenuHeader4X[3], kMenuHeaderY);

    lcd_drawMetro_ActiveData();
    lcd_drawMetro_PrecountData();
    lcd_drawMetro_SampleData();
    lcd_drawMetro_VolumeData();
}

void Controller::lcd_drawMetro_ActiveData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (metronome.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawMetro_PrecountData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (metronome.precount) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);
}

void Controller::lcd_drawMetro_SampleData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kMetroSampleDataLibrary[metronome.sample].nameLongR, kMenuDataTextSize, kMenuData4X[2], kMenuDataY);
}

void Controller::lcd_drawMetro_VolumeData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kMetronomeVolumeDataLibrary[metronome.volume].nameLongR, kMenuDataTextSize, kMenuData4X[3], kMenuDataY);
}

// eq menu functions

void Controller::lcd_drawEqMenu() {
    lcd_drawMenuIcon(EQ_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    for (uint16_t i = 0; i < 6; i++) {
        lcd.drawText(kHeaderEqFreq[i], 2, kMenuHeader8X[i + 2] - 2, kMenuHeaderY);
    }

    lcd.setFont(FONT_05x07);
    for (uint8_t i = 0; i < 6; i++) {
        if ((i > 0) && (i < 5)) {
            lcd.drawText("Q ", 2, kMenuHeader8X[i + 2] - 2, 52);
        }
        lcd.drawText("HZ", 2, kMenuHeader8X[i + 2] - 2, 66);
        lcd.drawText("DB", 2, kMenuHeader8X[i + 2] - 2, 80);
    }

    lcd.setForeColor(WHITE);
    lcd.drawHLine(401, 33, 6);
    lcd.drawLine(406, 33, 411, 38);
    lcd.drawHLine(411, 38, 6);
    lcd.drawHLine(808, 38, 6);
    lcd.drawLine(813, 38, 818, 33);
    lcd.drawHLine(818, 33, 6);

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t xPos = kMenuData8X[i + 3] - 17;
        lcd.drawHLine(xPos, 38, 5);
        lcd.drawLine(xPos + 4, 38, xPos + 9, 33);
        lcd.drawLine(xPos + 9, 33, xPos + 14, 38);
        lcd.drawHLine(xPos + 14, 38, 5);
    }

    lcd_drawEq_ActiveData();
    lcd_drawEq_LowShelfData();
    lcd_drawEq_HighShelfData();
    for (uint8_t i = 0; i < 4; i++) {
        lcd_drawEq_PeakData(i);
    }
}

void Controller::lcd_drawEq_ActiveData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (eq.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawEq_LowShelfData() {
    lcd_setMenuDataState(WHITE);
    lcd.setFont(FONT_05x07);
    lcd.drawText(kEqFreqDataLibrary[eq.freqLowShelf].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 4, 66);
    lcd.drawText(kEqGainDataLibrary[eq.gainLowShelf].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 4, 80);
}

void Controller::lcd_drawEq_HighShelfData() {
    lcd_setMenuDataState(WHITE);
    lcd.setFont(FONT_05x07);
    lcd.drawText(kEqFreqDataLibrary[eq.freqHighShelf].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 4, 66);
    lcd.drawText(kEqGainDataLibrary[eq.gainHighShelf].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 4, 80);
}

void Controller::lcd_drawEq_PeakData(uint8_t peakNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.setFont(FONT_05x07);
    lcd.setForeColor(GRAY_60);
    lcd.drawText(kEqQDataLibrary[eq.qPeak[peakNum_]].nameShortR, kMenuDataShortTextSize, kMenuData8X[3 + peakNum_] + 4, 52);
    lcd.setForeColor(WHITE);
    lcd.drawText(kEqFreqDataLibrary[eq.freqPeak[peakNum_]].nameShortR, kMenuDataShortTextSize, kMenuData8X[3 + peakNum_] + 4, 66);
    lcd.drawText(kEqGainDataLibrary[eq.gainPeak[peakNum_]].nameShortR, kMenuDataShortTextSize, kMenuData8X[3 + peakNum_] + 4, 80);
}

// osc functions

void Controller::lcd_drawOsc0Menu(Osc &osc_) {
    lcd_drawMenuIcon(OSC_A0_MENU);

    lcd_setMenuHeaderState(CYAN);
    (osc_.number == 0) ? lcd.drawText("OA", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY) : lcd.drawText("OB", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);
    lcd.drawText(kHeaderWavetable, kMenuHeaderTextSize, kMenuHeader8X[2], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("LEV ", kMenuHeaderShortTextSize, kMenuHeader8X[4], kMenuHeaderY);
    lcd.drawText("TUNE", kMenuHeaderShortTextSize, kMenuHeader8X[5], kMenuHeaderY);
    lcd.drawText("PHA ", kMenuHeaderShortTextSize, kMenuHeader8X[6], kMenuHeaderY);
    lcd.drawText("NORM", kMenuHeaderShortTextSize, kMenuHeader8X[7], kMenuHeaderY);

    lcd.setAlignment(RIGHT);
    lcd.setForeColor(GRAY_50);
    lcd.drawText(" <", 2, kMenuData8X[4], kMenuHeaderY);
    lcd.setForeColor(CYAN);
    lcd.drawText(" >", 2, kMenuData8X[7], kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawOsc_ActiveData(osc_);
    lcd_drawOsc_WavetableData(osc_);
    lcd_drawOsc_LevelData(osc_);
    lcd_drawOsc_TuneData(osc_);
    lcd_drawOsc_PhaseData(osc_);
    lcd_drawOsc_NormalizeData(osc_);
}

void Controller::lcd_drawOsc1Menu(Osc &osc_) {
    lcd_drawMenuIcon(OSC_A1_MENU);

    lcd_setMenuHeaderState(CYAN);
    (osc_.number == 0) ? lcd.drawText("OA", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY) : lcd.drawText("OB", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);
    lcd.drawText(kHeaderWavetable, kMenuHeaderTextSize, kMenuHeader8X[2], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("STA ", kMenuHeaderShortTextSize, kMenuHeader8X[4], kMenuHeaderY);
    lcd.drawText("END ", kMenuHeaderShortTextSize, kMenuHeader8X[5], kMenuHeaderY);
    lcd.drawText("FL_X", kMenuHeaderShortTextSize, kMenuHeader8X[6], kMenuHeaderY);
    lcd.drawText("FL_Y", kMenuHeaderShortTextSize, kMenuHeader8X[7], kMenuHeaderY);

    lcd.setAlignment(RIGHT);
    lcd.setForeColor(CYAN);
    lcd.drawText("<", 1, kMenuData8X[4], kMenuHeaderY);
    lcd.setForeColor(GRAY_50);
    lcd.drawText(">", 1, kMenuData8X[7], kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawOsc_ActiveData(osc_);
    lcd_drawOsc_WavetableData(osc_);
    lcd_drawOsc_StartData(osc_);
    lcd_drawOsc_EndData(osc_);
    lcd_drawOsc_XFlipData(osc_);
    lcd_drawOsc_YFlipData(osc_);
}

void Controller::lcd_drawOscLfoMenu(Osc &osc_, Lfo &lfo_) {
    lcd_drawMenuIcon(OSC_A2_MENU);

    lcd_setMenuHeaderState(CYAN);
    (lfo_.number == 0) ? lcd.drawText("01", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY) : lcd.drawText("02", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);
    lcd.drawText(kHeaderType, kMenuHeaderTextSize, kMenuHeader8X[2], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("TAR ", kMenuHeaderShortTextSize, kMenuHeader8X[4], kMenuHeaderY);
    lcd.drawText("RATE", kMenuHeaderShortTextSize, kMenuHeader8X[5], kMenuHeaderY);
    lcd.drawText("DEP ", kMenuHeaderShortTextSize, kMenuHeader8X[6], kMenuHeaderY);
    lcd.drawText("LOOP", kMenuHeaderShortTextSize, kMenuHeader8X[7], kMenuHeaderY);

    lcd.setAlignment(RIGHT);
    lcd.drawText("  ", 1, kMenuData8X[4], kMenuHeaderY);
    lcd.drawText("  ", 1, kMenuData8X[7], kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawOscLfo_ActiveData(osc_, lfo_);
    lcd_drawOscLfo_TypeData(osc_, lfo_);
    lcd_drawOscLfo_TargetData(osc_, lfo_);
    lcd_drawOscLfo_RateData(osc_, lfo_);
    lcd_drawOscLfo_DepthData(osc_, lfo_);
    lcd_drawOscLfo_LoopData(osc_, lfo_);
}

void Controller::lcd_drawOsc_ActiveData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (osc_.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawOsc_WavetableData(Osc &osc_) {
    (osc_.wavetableLoaded == osc_.wavetableSelected) ? lcd_setMenuDataState(WHITE) : lcd_setMenuDataState(GRAY_60);
    lcd.drawText(osc_.wavetableSelectedData.nameShortR, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);

    lcd_setMenuNumState(CYAN);
    lcd.drawText(osc_.wavetableSelectedData.num, 4, kMenuData4X[1], kMenuHeaderY + 1);

    uint16_t xPos = kMenuData4X[1];
    uint16_t yPos = kMenuHeaderY + 23;
    if (osc_.wavetableSelected != -1) {
        if (osc_.wavetableSelectedReadError) {
            lcd.setForeColor(kLayerColorPalette[9]);
            lcd.drawText(" READ ERROR", 11, xPos, yPos);
        } else if (osc_.wavetableSelectedTypeError) {
            lcd.setForeColor(kLayerColorPalette[9]);
            lcd.drawText(" TYPE ERROR", 11, xPos, yPos);
        } else {
            lcd.setSpacing(2);
            lcd.setForeColor(GRAY_60);
            lcd.drawText(kStepDataLibrary[osc_.wavetableSelectedData.step].name, 6, xPos - 35, yPos);
            lcd.drawText(
                kBitdepthDataLibrary[osc_.wavetableSelectedData.bitdepth].name, 2, xPos - 14, yPos);
            lcd.drawText(kChannelDataLibrary[osc_.wavetableSelectedData.channel].name, 1, xPos, yPos);
            lcd.drawText("-", 1, xPos - 28, yPos);
            lcd.drawText("-", 1, xPos - 7, yPos);
        }
    } else {
        lcd.clearRect(xPos - 80, yPos, 80, 8);
    }

    // lcd.drawNumber(layer.playSampleSector, 2, 50, 50);
    // lcd.drawNumber(layer.sampleSector[layer.playSampleSector].size, 5, 50, 65);
}

void Controller::lcd_drawOsc_LevelData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    char kData[kMenuDataShortTextSize + 1];
    (osc_.level == kMaxOscLevel) ? sprintf(kData, " %03d", osc_.level) : sprintf(kData, "  %02d", osc_.level);
    lcd.drawText(kData, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawOsc_TuneData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kOscTuneDataLibrary[osc_.tune].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
}

void Controller::lcd_drawOsc_PhaseData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kOscPhaseDataLibrary[osc_.phase].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawOsc_NormalizeData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (osc_.normalize) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

void Controller::lcd_drawOsc_StartData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    if (osc_.wavetableLoaded != -1) {
        char text[5];
        (osc_.waveStart >= 99) ? sprintf(text, " %03d", osc_.waveStart + 1) : sprintf(text, "  %02d", osc_.waveStart + 1);
        lcd.drawText(text, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
    } else {
        lcd.drawText("  --", kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
    }
}

void Controller::lcd_drawOsc_EndData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    if (osc_.wavetableLoaded != -1) {
        char text[5];
        (osc_.waveEnd >= 99) ? sprintf(text, " %03d", osc_.waveEnd + 1) : sprintf(text, "  %02d", osc_.waveEnd + 1);
        lcd.drawText(text, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
    } else {
        lcd.drawText(" --", kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
    }
}

void Controller::lcd_drawOsc_XFlipData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (osc_.xFlip) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawOsc_YFlipData(Osc &osc_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (osc_.yFlip) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

void Controller::lcd_drawOscLfo_ActiveData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (lfo_.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawOscLfo_TypeData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLfoTypeDataLibrary[lfo_.type].nameLongR, kMenuDataTextSize, kMenuData4X[1], kMenuDataY);
}

void Controller::lcd_drawOscLfo_TargetData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLfoTargetDataLibrary[lfo_.target].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawOscLfo_RateData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLfoRateDataLibrary[lfo_.rate].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
}

void Controller::lcd_drawOscLfo_DepthData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kLfoDepthDataLibrary[lfo_.depth].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawOscLfo_LoopData(Osc &osc_, Lfo &lfo_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (lfo_.loop) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

// filter menu functions

void Controller::lcd_drawFilterMenu(uint8_t filterNum_) {
    lcd_drawMenuIcon(FILTER_0_MENU);

    lcd_setMenuHeaderState(CYAN);
    (filterNum_ == 0) ? lcd.drawText("01", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY) : lcd.drawText("02", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("TYPE", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd.drawText("FREQ", kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd.drawText("RES ", kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd.drawText("SLO ", kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd.drawText("DRY ", kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd.drawText("WET ", kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[2] + 2, 60);
    lcd.drawText(" HZ", kMenuSignTextSize, kMenuData8X[3] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText(" DB", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawFilter_ActiveData(filterNum_);
    lcd_drawFilter_TypeData(filterNum_);
    lcd_drawFilter_FreqData(filterNum_);
    lcd_drawFilter_ResData(filterNum_);
    lcd_drawFilter_SlopeData(filterNum_);
    lcd_drawFilter_DryData(filterNum_);
    lcd_drawFilter_WetData(filterNum_);
}

void Controller::lcd_drawFilter_ActiveData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (filter[filterNum_].active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawFilter_TypeData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterTypeDataLibrary[filter[filterNum_].type].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawFilter_FreqData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterFreqDataLibrary[filter[filterNum_].freq].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
}

void Controller::lcd_drawFilter_ResData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterResDataLibrary[filter[filterNum_].res].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawFilter_SlopeData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterSlopeDataLibrary[filter[filterNum_].slope].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
}

void Controller::lcd_drawFilter_DryData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterMixDataLibrary[filter[filterNum_].dry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawFilter_WetData(uint8_t filterNum_) {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kFilterMixDataLibrary[filter[filterNum_].wet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

// envelope menu functions

void Controller::lcd_drawEnvelopeMenu() {
    lcd_drawMenuIcon(ENVELOPE_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("TYPE", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd.drawText("CUR ", kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd.drawText("ATT ", kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd.drawText("DEC ", kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd.drawText("SUS ", kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd.drawText("REL ", kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[2] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[3] + 2, 60);
    lcd.drawText("SEC", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("SEC", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("SEC", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawEnvelope_ActiveData();
    lcd_drawEnvelope_TypeData();
    lcd_drawEnvelope_CurveData();
    lcd_drawEnvelope_AttackTimeData();
    lcd_drawEnvelope_DecayTimeData();
    lcd_drawEnvelope_SustainLevelData();
    lcd_drawEnvelope_ReleaseTimeData();
}

void Controller::lcd_drawEnvelope_ActiveData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (envelope.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawEnvelope_TypeData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEnvelopeTypeDataLibrary[envelope.type].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawEnvelope_CurveData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEnvelopeCurveDataLibrary[envelope.curve].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
}

void Controller::lcd_drawEnvelope_AttackTimeData() {
    lcd_setMenuDataState(WHITE);
    switch (envelope.type) {
    case ENV_ADSR:
    case ENV_ASR:
    case ENV_AD:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.attackTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case ENV_OFF:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEnvelope_DecayTimeData() {
    lcd_setMenuDataState(WHITE);
    switch (envelope.type) {
    case ENV_ADSR:
    case ENV_AD:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.decayTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case ENV_OFF:
    case ENV_ASR:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEnvelope_SustainLevelData() {
    lcd_setMenuDataState(WHITE);
    switch (envelope.type) {
    case ENV_ADSR:
        lcd.drawText(kEnvelopeLevelDataLibrary[envelope.sustainLevel].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case ENV_OFF:
    case ENV_ASR:
    case ENV_AD:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEnvelope_ReleaseTimeData() {
    lcd_setMenuDataState(WHITE);
    switch (envelope.type) {
    case ENV_ADSR:
    case ENV_ASR:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.releaseTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case ENV_OFF:
    case ENV_AD:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;
    }
}

// effect menu functions

void Controller::lcd_drawEffectMenu(uint8_t effectNum_) {
    lcd_drawMenuIcon(EFFECT_0_MENU);

    lcd_setMenuHeaderState(CYAN);
    (effectNum_ == 0) ? lcd.drawText("01", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY) : lcd.drawText("02", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd_drawEffect_ActiveData(effectNum_);
    lcd_drawEffect_TypeData(effectNum_);
    lcd_drawEffect_AData(effectNum_);
    lcd_drawEffect_BData(effectNum_);
    lcd_drawEffect_CData(effectNum_);
    lcd_drawEffect_DData(effectNum_);
    lcd_drawEffect_EData(effectNum_);
}

void Controller::lcd_drawEffect_ActiveData(uint8_t effectNum_) {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (effect[effectNum_].active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawEffect_TypeData(uint8_t effectNum_) {
    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("TYPE", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[2] + 2, 60);
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kEffectTypeDataLibrary[effect[effectNum_].type].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawEffect_AData(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText(aHeader[type_], kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(aSign[type_], kMenuSignTextSize, kMenuData8X[3] + 2, 60);
    lcd_setMenuDataState(WHITE);

    switch (type_) {
    case EF_DELAY:
        lcd.drawText(kDelayTimeDataLibrary[effect_.delay.aTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_CHORUS:
        lcd.drawText(kChorusTimeDataLibrary[effect_.chorus.aTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_FLANGER:
        lcd.drawText(kFlangerTimeDataLibrary[effect_.flanger.aTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_PHASER:
        lcd.drawText(kPhaserFreqDataLibrary[effect_.phaser.aStartFreq].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_COMPRESSOR:
        lcd.drawText(
            kCompressorThresholdDataLibrary[effect_.compressor.aThreshold].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_EXPANDER:
        lcd.drawText(
            kExpanderThresholdDataLibrary[effect_.expander.aThreshold].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_OVERDRIVE:
        lcd.drawText(
            kOverdriveGainDataLibrary[effect_.overdrive.aGain].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_DISTORTION:
        lcd.drawText(
            kDistortionGainDataLibrary[effect_.distortion.aGain].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;

    case EF_BITCRUSHER:
        lcd.drawText(
            kBitcrusherResolutionDataLibrary[effect_.bitcrusher.aResolution].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEffect_BData(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText(bHeader[type_], kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(bSign[type_], kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd_setMenuDataState(WHITE);

    switch (type_) {
    case EF_DELAY:
        lcd.drawText(
            kDelayLevelDataLibrary[effect_.delay.bLevel].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_CHORUS:
        lcd.drawText(
            kChorusFeedbackDataLibrary[effect_.chorus.bFeedback].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_FLANGER:
        lcd.drawText(
            kFlangerFeedbackDataLibrary[effect_.flanger.bFeedback].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_PHASER:
        lcd.drawText(kPhaserFreqDataLibrary[effect_.phaser.bEndFreq].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_COMPRESSOR:
        lcd.drawText(
            kCompressorRateDataLibrary[effect_.compressor.bRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_EXPANDER:
        lcd.drawText(kExpanderRateDataLibrary[effect_.expander.bRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_OVERDRIVE:
        lcd.drawText(kOverdriveThresholdDataLibrary[effect_.overdrive.bThreshold].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_DISTORTION:
        lcd.drawText(
            kDistortionThresholdDataLibrary[effect_.distortion.bThreshold].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;

    case EF_BITCRUSHER:
        lcd.drawText(
            kBitcrusherSampleRateDataLibrary[effect_.bitcrusher.bSampleRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEffect_CData(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText(cHeader[type_], kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(cSign[type_], kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd_setMenuDataState(WHITE);

    switch (type_) {
    case EF_DELAY:
        lcd.drawText(kDelayFeedbackDataLibrary[effect_.delay.cFeedback].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_CHORUS:
        lcd.drawText(kChorusRateDataLibrary[effect_.chorus.cRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_FLANGER:
        lcd.drawText(kFlangerRateDataLibrary[effect_.flanger.cRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_PHASER:
        lcd.drawText(kPhaserRateDataLibrary[effect_.phaser.cRate].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_COMPRESSOR:
        lcd.drawText(
            kCompressorAttackTimeDataLibrary[effect_.compressor.cAttackTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_EXPANDER:
        lcd.drawText(kExpanderAttackTimeDataLibrary[effect_.expander.cAttackTime].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_OVERDRIVE:
        lcd.drawText(
            kOverdriveToneDataLibrary[effect_.overdrive.cTone].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_DISTORTION:
        lcd.drawText(
            kDistortionToneDataLibrary[effect_.distortion.cTone].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case EF_BITCRUSHER:
        lcd.drawText(
            kBitcrusherThresholdDataLibrary[effect_.bitcrusher.cThreshold].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEffect_DData(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText(dHeader[type_], kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(dSign[type_], kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd_setMenuDataState(WHITE);

    switch (type_) {
    case EF_DELAY:
        lcd.drawText(kEffectMixDataLibrary[effect_.delay.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_CHORUS:
        lcd.drawText(kEffectMixDataLibrary[effect_.chorus.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_FLANGER:
        lcd.drawText(kEffectMixDataLibrary[effect_.flanger.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_PHASER:
        lcd.drawText(kEffectMixDataLibrary[effect_.phaser.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_COMPRESSOR:
        lcd.drawText(
            kCompressorReleaseTimeDataLibrary[effect_.compressor.dReleaseTime].nameShortR,
            kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_EXPANDER:
        lcd.drawText(
            kExpanderReleaseTimeDataLibrary[effect_.expander.dReleaseTime].nameShortR,
            kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_OVERDRIVE:
        lcd.drawText(kEffectMixDataLibrary[effect_.overdrive.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_DISTORTION:
        lcd.drawText(kEffectMixDataLibrary[effect_.distortion.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;

    case EF_BITCRUSHER:
        lcd.drawText(kEffectMixDataLibrary[effect_.bitcrusher.dDry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawEffect_EData(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText(eHeader[type_], kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);
    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText(eSign[type_], kMenuSignTextSize, kMenuData8X[7] + 2, 60);
    lcd_setMenuDataState(WHITE);

    switch (type_) {
    case EF_DELAY:
        lcd.drawText(kEffectMixDataLibrary[effect_.delay.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_CHORUS:
        lcd.drawText(kEffectMixDataLibrary[effect_.chorus.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_FLANGER:
        lcd.drawText(kEffectMixDataLibrary[effect_.flanger.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_PHASER:
        lcd.drawText(kEffectMixDataLibrary[effect_.phaser.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_COMPRESSOR:
        lcd.drawText(kEffectMixDataLibrary[effect_.compressor.eMix].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_EXPANDER:
        lcd.drawText(kEffectMixDataLibrary[effect_.expander.eMix].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_OVERDRIVE:
        lcd.drawText(kEffectMixDataLibrary[effect_.overdrive.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_DISTORTION:
        lcd.drawText(kEffectMixDataLibrary[effect_.distortion.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;

    case EF_BITCRUSHER:
        lcd.drawText(kEffectMixDataLibrary[effect_.bitcrusher.eWet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
        break;
    }
}

// reverb menu functions

void Controller::lcd_drawReverbMenu() {
    lcd_drawMenuIcon(REVERB_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderActive, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("SIZE", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd.drawText("DEC ", kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd.drawText("PRE ", kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd.drawText("SUR ", kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd.drawText("DRY ", kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd.drawText("WET ", kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[2] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[3] + 2, 60);
    lcd.drawText(" MS", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("  %", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawReverb_ActiveData();
    lcd_drawReverb_SizeData();
    lcd_drawReverb_DecayData();
    lcd_drawReverb_PreDelayData();
    lcd_drawReverb_SurroundData();
    lcd_drawReverb_DryData();
    lcd_drawReverb_WetData();
}

void Controller::lcd_drawReverb_ActiveData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (reverb.active) ? textPtr = kDataOn : textPtr = kDataOff;
    lcd.drawText(textPtr, kMenuDataTextSize, kMenuData8X[1], kMenuDataY);
}

void Controller::lcd_drawReverb_SizeData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbSizeDataLibrary[reverb.size].nameShortR, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawReverb_DecayData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbMixDataLibrary[reverb.decay].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
}

void Controller::lcd_drawReverb_PreDelayData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbPreDelayDataLibrary[reverb.preDelay].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawReverb_SurroundData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbSurroundDataLibrary[reverb.surround].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
}

void Controller::lcd_drawReverb_DryData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbMixDataLibrary[reverb.dry].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawReverb_WetData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kReverbMixDataLibrary[reverb.wet].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

// key menu functions

void Controller::lcd_drawKeyMenu() {
    lcd_drawMenuIcon(KEY_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderKey, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("ARP ", kMenuHeaderShortTextSize, kMenuHeader8X[2] - 2, kMenuHeaderY);
    lcd.drawText("RATE", kMenuHeaderShortTextSize, kMenuHeader8X[3] - 2, kMenuHeaderY);
    lcd.drawText("OSC ", kMenuHeaderShortTextSize, kMenuHeader8X[4] - 2, kMenuHeaderY);
    lcd.drawText("CHO ", kMenuHeaderShortTextSize, kMenuHeader8X[5] - 2, kMenuHeaderY);
    lcd.drawText("ORD ", kMenuHeaderShortTextSize, kMenuHeader8X[6] - 2, kMenuHeaderY);
    lcd.drawText("OCT ", kMenuHeaderShortTextSize, kMenuHeader8X[7] - 2, kMenuHeaderY);

    lcd_setMenuSignState(CYAN, FONT_05x07);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[2] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[3] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[4] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[5] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[6] + 2, 60);
    lcd.drawText("   ", kMenuSignTextSize, kMenuData8X[7] + 2, 60);

    lcd_drawKey_NoteData();
    lcd_drawKey_ArpegData();
    lcd_drawKey_RateData();
    lcd_drawKey_OscData();
    lcd_drawKey_ChordData();
    lcd_drawKey_OrderData();
    lcd_drawKey_OctaveData();
}

void Controller::lcd_drawKey_NoteData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kKeyNoteDataLibrary[key.note].nameLongR, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);
}

void Controller::lcd_drawKey_ArpegData() {
    lcd_setMenuDataState(WHITE);
    const char *textPtr;
    (key.arpeg) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, kMenuData8X[2] + 2, kMenuDataY);
}

void Controller::lcd_drawKey_RateData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kKeyRateDataLibrary[key.rate].nameShortR, kMenuDataShortTextSize, kMenuData8X[3] + 2, kMenuDataY);
}

void Controller::lcd_drawKey_OscData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kKeyOscDataLibrary[key.osc].nameShortR, kMenuDataShortTextSize, kMenuData8X[4] + 2, kMenuDataY);
}

void Controller::lcd_drawKey_ChordData() {
    lcd_setMenuDataState(WHITE);
    switch (key.chordType) {
    case CHORD_3:
        lcd.drawText(kKeyChord3DataLibrary[key.chord].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;

    case CHORD_4:
        lcd.drawText(kKeyChord4DataLibrary[key.chord].nameShortR, kMenuDataShortTextSize, kMenuData8X[5] + 2, kMenuDataY);
        break;
    }
}

void Controller::lcd_drawKey_OrderData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kKeyOrderDataLibrary[key.order].nameShortR, kMenuDataShortTextSize, kMenuData8X[6] + 2, kMenuDataY);
}

void Controller::lcd_drawKey_OctaveData() {
    lcd_setMenuDataState(WHITE);
    lcd.drawText(kKeyOctaveDataLibrary[key.octave].nameShortR, kMenuDataShortTextSize, kMenuData8X[7] + 2, kMenuDataY);
}

// song menu functions

void Controller::lcd_drawSongMenu() {
    lcd_drawMenuIcon(SONG_MENU);

    lcd_setMenuHeaderState(CYAN);
    lcd.drawText("  ", kMenuNumberTextSize, kMenuNumberX, kMenuNumberY);

    lcd_setMenuHeaderState(WHITE);
    lcd.drawText(kHeaderNote, kMenuHeaderTextSize, kMenuHeader8X[0], kMenuHeaderY);

    lcd_drawSong_BeatData();
    lcd_drawSong_BeatGraph();

    if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)
        lcd_drawBeat(activeBankNum, 0, true);
}

void Controller::lcd_drawSong_BeatData() {
    lcd_setMenuDataState(WHITE);
    Bank &bank = song.bankLibrary[activeBankNum];
    const char *text;
    if ((selectedBeatNum != -1) && (key.note)) {
        Beat &beat = bank.beatLibrary[selectedBeatNum];
        uint8_t note = ((key.note - 1) + beat.note) % 12;
        uint8_t octave = beat.octave - 1;
        char dataText[11];
        const char *noteText;
        const char *octaveText;
        noteText = kNoteTextDataLibrary[note];
        octaveText = kOctaveTextDataLibrary[octave];
        strcpy(dataText, noteText);
        strcat(dataText, octaveText);
        text = dataText;
    } else {
        text = kDataDashR;
    }
    lcd.drawText(text, kMenuDataTextSize, kMenuData4X[0], kMenuDataY);

    lcd_setMenuNumState(CYAN);
    char numText[6];
    if (bank.lastActiveBeatNum != -1) {
        char num0[3];
        char num1[3];
        sprintf(num0, "%02d", selectedBeatNum + 1);
        sprintf(num1, "%02d", bank.lastActiveBeatNum + 1);
        strcpy(numText, num0);
        strcat(numText, "|");
        strcat(numText, num1);
    } else {
        strcpy(numText, "--|--");
    }
    lcd.drawText(numText, 5, kMenuData4X[0], kMenuHeaderY + 1);
}

void Controller::lcd_drawSong_BeatGraph() {
    Bank &bank = song.bankLibrary[activeBankNum];

    lcd.setForeColor(WHITE);
    lcd.setBackColor(BLACK);
    lcd.setAlignment(LEFT);
    lcd.setFont(FONT_05x07);

    // draw text
    if (selectedBeatNum != -1) {
        char *text;
        Beat &beat = bank.beatLibrary[selectedBeatNum];
        uint16_t startInterval = beat.startInterval;
        uint16_t endInterval = beat.endInterval;
        uint8_t startBar = startInterval / barInterval;
        uint8_t startMeasure = (startInterval % barInterval) / measureInterval;
        uint8_t startRemainder = ((float)(startInterval % measureInterval) / measureInterval) * 100;
        uint8_t endBar = endInterval / barInterval;
        uint8_t endMeasure = (endInterval % barInterval) / measureInterval;
        uint8_t endRemainder = ((float)(endInterval % measureInterval) / measureInterval) * 100;

        lcd.drawText(kDataDot, 1, kBeatGraphStartTimeX + 14, kBeatGraphTimeY);
        lcd.drawText(kDataDot, 1, kBeatGraphStartTimeX + 35, kBeatGraphTimeY);
        sprintf(text, "%02d", startBar);
        lcd.drawText(text, 2, kBeatGraphStartTimeX, kBeatGraphTimeY);
        sprintf(text, "%02d", startMeasure);
        lcd.drawText(text, 2, kBeatGraphStartTimeX + 21, kBeatGraphTimeY);
        sprintf(text, "%02d", startRemainder);
        lcd.drawText(text, 2, kBeatGraphStartTimeX + 42, kBeatGraphTimeY);

        lcd.drawText(kDataDot, 1, kBeatGraphEndTimeX + 14, kBeatGraphTimeY);
        lcd.drawText(kDataDot, 1, kBeatGraphEndTimeX + 35, kBeatGraphTimeY);
        sprintf(text, "%02d", endBar);
        lcd.drawText(text, 2, kBeatGraphEndTimeX, kBeatGraphTimeY);
        sprintf(text, "%02d", endMeasure);
        lcd.drawText(text, 2, kBeatGraphEndTimeX + 21, kBeatGraphTimeY);
        sprintf(text, "%02d", endRemainder);
        lcd.drawText(text, 2, kBeatGraphEndTimeX + 42, kBeatGraphTimeY);
    } else {
        // lcd.clearRect(420, 28, 32, 5);
        lcd.drawText(kDataTimeBlank, 8, kBeatGraphStartTimeX, kBeatGraphTimeY);
        lcd.drawText(kDataTimeBlank, 8, kBeatGraphEndTimeX, kBeatGraphTimeY);
    }

    // draw graph
    if (selectedBeatNum != -1) {
        Beat &beat = bank.beatLibrary[selectedBeatNum];

        // clear graph
        // lcd.clearRect(kBeatGraphStartX + 1, kBeatGraphStartY, kBeatGraphWidth -
        // 2, kBeatGraphHeight - 1); draw graph base
        lcd.drawVLine(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphHeight);
        lcd.drawVLine(kBeatGraphEndX, kBeatGraphStartY, kBeatGraphHeight);
        lcd.drawHLine(kBeatGraphStartX, kBeatGraphEndY, kBeatGraphWidth);
        for (uint8_t i = 0; i < 18; i++) {
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 2), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (3 * kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
        }
        // draw graph data
        lcd.setForeColor(WHITE);
        uint16_t xGraph = kBeatGraphStartX;
        uint16_t yGraph = kBeatGraphStartY;
        lcd.fillRect(xGraph, yGraph + kBeatGraphHeight - 5, kBeatGraphWidth, 5);
    } else {
        // clear graph
        lcd.clearRect(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphWidth - 1, kBeatGraphHeight - 1);
        // draw graph base
        lcd.drawVLine(kBeatGraphStartX, kBeatGraphStartY, kBeatGraphHeight);
        lcd.drawVLine(kBeatGraphEndX, kBeatGraphStartY, kBeatGraphHeight);
        lcd.drawHLine(kBeatGraphStartX, kBeatGraphEndY, kBeatGraphWidth);
        for (uint8_t i = 0; i < 18; i++) {
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (kBeatGraphWidth / 2), kBeatGraphStartY + (i * 3));
            lcd.drawPixel(kBeatGraphStartX + (3 * kBeatGraphWidth / 4), kBeatGraphStartY + (i * 3));
        }
    }
}

// bank functions

void Controller::lcd_drawBank(uint8_t bankNum_, bool mode_) {
    lcd.setAlignment(LEFT);
    lcd.setFont(FONT_05x07);
    lcd.setBackColor(BLACK);
    (mode_) ? lcd.setForeColor(GREEN) : lcd.setForeColor(YELLOW);
    lcd.drawText(kNumberDataLibrary[bankNum_ + 1].nameShortL, 2, 114, 110);
}

void Controller::lcd_drawBankShift() {
    if (bankActionFlag) {
        bankActionFlag = false;
        lcd_drawBank(activeBankNum, false);
        for (uint8_t i = 0; i < kBankLibrarySize; i++) {
            lcd_drawSong(i);
        }

        if (menu == SONG_MENU) {
            (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;
            if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)
                lcd_drawBeat(activeBankNum, 0, true);
            lcd_drawSong_BeatData();
            lcd_drawSong_BeatGraph();
        }
    }
}

// octave functions

void Controller::lcd_drawOctave() {
    for (uint8_t i = 0; i < 5; i++) {
        (activeOctaveNum == (i + 2)) ? lcd.setForeColor(WHITE) : lcd.setForeColor(0x632C);
        lcd.setBackColor(BLACK);

        uint16_t xPos = kOctaveX[i];
        uint16_t yPos = kOctaveY;

        lcd.fillRect(xPos + 7, yPos, 2, 2);
        lcd.fillRect(xPos, yPos + 4, 16, 3);
    }
}

// transition functions

void Controller::lcd_drawTransition() {
    if (transitionShowFlag) {
        lcd.setFont(FONT_05x07);
        lcd.setAlignment(RIGHT);
        (transitionShowFlag == 1) ? lcd.setForeColor(MAGENTA) : lcd.setForeColor(GREEN);
        lcd.setBackColor(BLACK);
        lcd.drawText("PASS", 4, kMenuData8X[1], kMenuHeaderY + 2);

        transitionShowFlag = 0;
    }

    if (transitionClearFlag) {
        lcd.setFont(FONT_05x07);
        lcd.setAlignment(RIGHT);
        lcd.setForeColor(MAGENTA);
        lcd.setBackColor(BLACK);
        lcd.drawText("    ", 4, kMenuData8X[1], kMenuHeaderY + 2);

        transitionClearFlag = false;
    }
}

// play functions

void Controller::lcd_drawPlay() {
    if ((playActive) && (!metronome.precountState)) {
        if (((playInterval / playXRatio) > playX) && (playX < kPlayWidth)) {
            lcd.setForeColor(playColor);
            lcd.drawVLine(kPlayX + playX, kPlayY, 1);
            playX += 1;
        }
    }

    if (resetPlayFlag) {
        lcd_resetPlay();
        resetPlayFlag = false;
    }
}

void Controller::lcd_drawIcon() {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;
    uint16_t xPos;

    indexPtr = (const RGB16Color *)(RAM_ICON_PLAY_PALETTE_ADDRESS);

    if (resetIcon.flag) {
        (resetIcon.mode)
            ? dataPtr = (const uint8_t *)(RAM_ICON_PLAY_RESET_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_ICON_PLAY_RESET_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconResetX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        resetIcon.flag = false;
    }

    if (playIcon.flag) {
        (playIcon.mode)
            ? dataPtr = (const uint8_t *)(RAM_ICON_PLAY_PLAY_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_ICON_PLAY_PLAY_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconPlayX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        playIcon.flag = false;
    }

    if (stopIcon.flag) {
        (stopIcon.mode)
            ? dataPtr = (const uint8_t *)(RAM_ICON_PLAY_STOP_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_ICON_PLAY_STOP_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconStopX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        stopIcon.flag = false;
    }

    if (recordIcon.flag) {
        (recordIcon.mode)
            ? dataPtr = (const uint8_t *)(RAM_ICON_PLAY_RECORD_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_ICON_PLAY_RECORD_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kIconRecordX, kIconPlayY, kIconPlayWidth, kIconPlayHeight);
        recordIcon.flag = false;
    }
}

void Controller::lcd_drawText() {
    if (textCopyFlag) {
        textCopyFlag = false;
        textShow = true;
        lcd.setBackColor(BLACK);
        lcd.setAlignment(LEFT);
        lcd.setFont(FONT_05x07);
        lcd.setForeColor(GREEN);
        lcd.drawText("C", 1, 99, 110);
        startTextTimer();
    } else if (textPasteFlag) {
        textPasteFlag = false;
        textShow = true;
        lcd.setBackColor(BLACK);
        lcd.setAlignment(LEFT);
        lcd.setFont(FONT_05x07);
        lcd.setForeColor(YELLOW);
        lcd.drawText("P", 1, 99, 110);
        startTextTimer();
    } else if (textClearFlag) {
        textClearFlag = false;
        lcd.setBackColor(BLACK);
        lcd.setAlignment(LEFT);
        lcd.setFont(FONT_05x07);
        lcd.drawText(" ", 1, 99, 110);
    }
}

void Controller::lcd_drawCountDown() {
    if (metronome.countDownFlag) {
        if (metronome.countDown > 0) {
            lcd.setAlignment(LEFT);
            lcd.setFont(FONT_05x07);
            lcd.setBackColor(BLACK);
            lcd.setForeColor(kLayerColorPalette[9]);
            char kCountDownData[1];
            sprintf(kCountDownData, "%01d", metronome.countDown);
            lcd.drawText(kCountDownData, 1, 99, 110);
            metronome.countDown -= 1;
        } else {
            lcd_clearCountDown();
        }
        metronome.countDownFlag = false;
    }
}

void Controller::lcd_clearCountDown() {
    lcd.setBackColor(BLACK);
    lcd.clearRect(99, 110, 5, 7);
}

void Controller::lcd_restartPlay() {
    lcd_invertPlayColor();
    playX = 0;
}

void Controller::lcd_resetPlay() {
    playX = 0;
    lcd_calculateSongX();
    lcd.setForeColor(kPlayColor1);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);
    lcd_resetPlayColor();
    lcd_invertPlayColor();
}

void Controller::lcd_redrawPlay() {
    playX = playInterval / playXRatio;
    lcd.setForeColor(playColor);
    lcd.drawHLine(kPlayX, kPlayY, kPlayWidth);
    lcd_invertPlayColor();
}

void Controller::lcd_cleanEndPlay() {
    (lcd.getForeColor() == kPlayColor0) ? lcd.setForeColor(kPlayColor1) : lcd.setForeColor(kPlayColor0);
    lcd.drawHLine(kPlayX + playX + 1, kPlayY, kPlayWidth - playX);
}

void Controller::lcd_invertPlayColor() {
    (playColor == kPlayColor0) ? playColor = kPlayColor1 : playColor = kPlayColor0;
}

void Controller::lcd_resetPlayColor() {
    playColor = kPlayColor1;
}

// song functions

void Controller::lcd_calculateSongX() {
    playXRatio = (float)songInterval / kPlayWidth;
}

void Controller::lcd_drawMeasureBar() {
    float barX = (float)kPlayWidth / rhythm.bar;
    float measureX = (float)kPlayWidth / (rhythm.measure * rhythm.bar);
    lcd.setBackColor(BLACK);
    lcd.setForeColor(WHITE);
    lcd.clearRect(kPlayX + 3, kPlayY + 7, kPlayWidth - 6, 3);
    lcd.clearRect(kPlayX + 3, kPlayY + 61, kPlayWidth - 6, 3);
    for (uint8_t i = 1; i < (rhythm.measure * rhythm.bar); i++) {
        uint16_t posX = kPlayX + int(i * measureX);
        if (i % rhythm.measure == 0) {
            lcd.fillRect(posX - 1, kPlayY + 7, 3, 3);
            lcd.fillRect(posX - 1, kPlayY + 61, 3, 3);
        } else {
            lcd.fillRect(posX, kPlayY + 7, 1, 3);
            lcd.fillRect(posX, kPlayY + 61, 1, 3);
        }
    }
}

void Controller::lcd_drawStartBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_) {
    if (bankNum_ == activeBankNum) {
        Beat &beat = song.bankLibrary[bankNum_].beatLibrary[beatNum_];
        // calculate intervals
        uint16_t startInterval = beat.startInterval;
        // calculate x - y positions
        uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
        uint16_t yStart = kSongY + (kNoteHeight * (12 - beat.note));
        // draw beat
        RGB16Color color;
        (selected_) ? color = RED : color = YELLOW;
        lcd.setForeColor(color);
        lcd.drawVLine(xStart, yStart, kNoteHeight);
    }
}

void Controller::lcd_drawEndBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_) {
    if (bankNum_ == activeBankNum) {
        Beat &beat = song.bankLibrary[bankNum_].beatLibrary[beatNum_];
        // calculate intervals
        uint16_t startInterval = beat.startInterval;
        uint16_t endInterval = beat.endInterval;
        // calculate x - y positions
        uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
        uint16_t xEnd = kSongX + (endInterval * kSongWidth / songInterval);
        uint16_t yStart = kSongY + (kNoteHeight * (12 - beat.note));
        // draw beat
        RGB16Color color;
        (selected_) ? color = RED : color = YELLOW;
        lcd.setForeColor(color);
        lcd.fillRect(xStart, yStart, xEnd - xStart - 2, kNoteHeight);
    }
}

void Controller::lcd_drawBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_) {
    if (bankNum_ == activeBankNum) {
        Beat &beat = song.bankLibrary[bankNum_].beatLibrary[beatNum_];
        // calculate intervals
        uint16_t startInterval = beat.startInterval;
        uint16_t endInterval = beat.endInterval;
        // calculate x - y positions
        uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
        uint16_t xEnd = kSongX + (endInterval * kSongWidth / songInterval);
        uint16_t yStart = kSongY + (kNoteHeight * (12 - beat.note));
        // draw beat
        RGB16Color color;
        (selected_) ? color = RED : color = YELLOW;
        lcd.setForeColor(color);
        lcd.fillRect(xStart, yStart, xEnd - xStart - 2, kNoteHeight);
    }
}

void Controller::lcd_clearBeat(uint8_t bankNum_, uint8_t beatNum_) {
    if (bankNum_ == activeBankNum) {
        Beat &beat = song.bankLibrary[bankNum_].beatLibrary[beatNum_];
        if (beat.active) {
            // calculate intervals
            uint16_t startInterval = beat.startInterval;
            uint16_t endInterval = beat.endInterval;
            // calculate x - y positions
            uint16_t xStart = kSongX + (startInterval * kSongWidth / songInterval);
            uint16_t xEnd = kSongX + (endInterval * kSongWidth / songInterval);
            uint16_t yStart = kSongY + (kNoteHeight * (12 - beat.note));
            // clear beat
            RGB16Color color;
            (beat.note % 2) ? color = 0x0000 : color = 0x528A;
            lcd.setForeColor(color);
            lcd.fillRect(xStart, yStart, xEnd - xStart, kNoteHeight);
        }
    }
}

void Controller::lcd_drawSong(uint8_t bankNum_) {
    if (bankNum_ == activeBankNum) {
        lcd_clearSong();
        Bank &bank = song.bankLibrary[bankNum_];
        if (bank.lastActiveBeatNum != -1) {
            for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                Beat &beat = bank.beatLibrary[i];
                if (beat.active)
                    lcd_drawBeat(bankNum_, i, false);
            }
        }
    }
}

void Controller::lcd_clearSong() {
    for (uint8_t i = 0; i < 13; i++) {
        RGB16Color color;
        (i % 2) ? color = 0x00 : color = 0x528A;
        lcd.setForeColor(color);
        lcd.fillRect(kSongX, kSongY + (i * kNoteHeight), kSongWidth, kNoteHeight);
    }
}

void Controller::lcd_clearBeatInterval(uint8_t bankNum_, uint8_t beatNum_, uint16_t startInterval_, uint16_t endInterval_) {
    if (bankNum_ == activeBankNum) {
        Beat &beat = song.bankLibrary[bankNum_].beatLibrary[beatNum_];
        if (beat.active) {
            // calculate x - y positions
            uint16_t xStart = kSongX + (startInterval_ * kSongWidth / songInterval);
            uint16_t xEnd = kSongX + (endInterval_ * kSongWidth / songInterval);
            uint16_t yStart = kSongY + (kNoteHeight * (12 - beat.note));
            // clear interval
            RGB16Color color;
            (beat.note % 2) ? color = 0x0000 : color = 0x528A;
            lcd.setForeColor(color);
            lcd.fillRect(xStart - 2, yStart, xEnd - xStart + 2, kNoteHeight);
        }
    }
}

// info functions

// info song

void Controller::lcd_drawInfo_Song_Select(bool selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    palettePtr = (const RGB16Color *)(RAM_BUTTON_SONG_PALETTE_ADDRESS);
    (selected) ? dataPtr = (const uint8_t *)(RAM_BUTTON_SONG_ON_DATA_ADDRESS) : dataPtr = (const uint8_t *)(RAM_BUTTON_SONG_OFF_DATA_ADDRESS);

    lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonSongX, kButtonSongY, kButtonSongWidth, kButtonSongHeight);
}

// info key

void Controller::lcd_drawInfo_Key_Select(bool selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    palettePtr = (const RGB16Color *)(RAM_BUTTON_KEY_PALETTE_ADDRESS);
    (selected) ? dataPtr = (const uint8_t *)(RAM_BUTTON_KEY_ON_DATA_ADDRESS) : dataPtr = (const uint8_t *)(RAM_BUTTON_KEY_OFF_DATA_ADDRESS);

    lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonKeyX, kButtonKeyY, kButtonKeyWidth, kButtonKeyHeight);
}

void Controller::lcd_drawInfo_Key_NoteData() {
    uint16_t xPos = kInfoStartX + 9;
    uint16_t yPos = kInfoStartY + 24;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kKeyNoteDataLibrary[key.note].nameShortL, kMenuDataShortTextSize, xPos, yPos);

    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    indexPtr = (const RGB16Color *)(RAM_GRAPH_KEY_PALETTE_ADDRESS);
    dataPtr = (const uint8_t *)(RAM_GRAPH_KEY_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * key.note));

    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoKeyGraphX, kInfoKeyGraphY, kInfoGraphWidth, kInfoGraphHeight);
}

void Controller::lcd_drawInfo_Key_ArpegData() {
    uint16_t xPos = kInfoStartX + 64;
    uint16_t yPos = kInfoStartY + 24;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    const char *textPtr;
    (key.arpeg) ? textPtr = kDataShortROn : textPtr = kDataShortROff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Key_RateData() {
    uint16_t xPos = kInfoStartX + 9;
    uint16_t yPos = kInfoStartY + 104;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kKeyRateDataLibrary[key.rate].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Key_OscData() {
    uint16_t xPos = kInfoStartX + 9;
    uint16_t yPos = kInfoStartY + 133;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kKeyOscDataLibrary[key.osc].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Key_ChordData() {
    uint16_t xPos = kInfoStartX + 64;
    uint16_t yPos = kInfoStartY + 133;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    switch (key.chordType) {
    case CHORD_3:
        lcd.drawText(kKeyChord3DataLibrary[key.chord].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;

    case CHORD_4:
        lcd.drawText(kKeyChord4DataLibrary[key.chord].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;
    }
}

void Controller::lcd_drawInfo_Key_OrderData() {
    uint16_t xPos = kInfoStartX + 9;
    uint16_t yPos = kInfoStartY + 213;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kKeyOrderDataLibrary[key.order].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Key_OctaveData() {
    uint16_t xPos = kInfoStartX + 64;
    uint16_t yPos = kInfoStartY + 213;

    RGB16Color foreColor = YELLOW;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kKeyOctaveDataLibrary[key.octave].nameShortR, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Key_PatternData() {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    if (key.arpeg) {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_ARPEG_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_ARPEG_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * key.chordPattern));
    } else {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_ARPEG_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_ARPEG_DATA_ADDRESS);
    }
    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoArpegGraphX, kInfoArpegGraphY, kInfoGraphWidth, kInfoGraphHeight);
}

// info osc

void Controller::lcd_drawInfo_Osc_Select(Osc &osc_, uint8_t selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    switch (osc_.number) {
    case 0:
        palettePtr = (const RGB16Color *)(RAM_BUTTON_OSC_A_PALETTE_ADDRESS);
        switch (selected) {
        case 0:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_A_0_DATA_ADDRESS);
            break;

        case 1:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_A_1_DATA_ADDRESS);
            break;

        case 2:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_A_2_DATA_ADDRESS);
            break;
        }
        lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonOscAX, kButtonOscAY, kButtonOscAWidth, kButtonOscAHeight);
        break;

    case 1:
        palettePtr = (const RGB16Color *)(RAM_BUTTON_OSC_B_PALETTE_ADDRESS);
        switch (selected) {
        case 0:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_B_0_DATA_ADDRESS);
            break;

        case 1:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_B_1_DATA_ADDRESS);
            break;

        case 2:
            dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_B_2_DATA_ADDRESS);
            break;
        }
        lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonOscBX, kButtonOscBY, kButtonOscBWidth, kButtonOscBHeight);
        break;
    }
}

void Controller::lcd_drawInfo_OscLfo_Select(Osc &osc_, Lfo &lfo_, bool selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;
    uint16_t xPos;

    switch (osc_.number) {
    case 0:
        palettePtr = (const RGB16Color *)(RAM_BUTTON_OSC_A_LFO_PALETTE_ADDRESS);
        (selected)
            ? dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_A_LFO_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_A_LFO_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonOscALfoX[lfo_.number], kButtonLfoY, kButtonLfoWidth, kButtonLfoHeight);
        break;

    case 1:
        palettePtr = (const RGB16Color *)(RAM_BUTTON_OSC_B_LFO_PALETTE_ADDRESS);
        (selected)
            ? dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_B_LFO_ON_DATA_ADDRESS)
            : dataPtr = (const uint8_t *)(RAM_BUTTON_OSC_B_LFO_OFF_DATA_ADDRESS);
        lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonOscBLfoX[lfo_.number], kButtonLfoY, kButtonLfoWidth, kButtonLfoHeight);
        break;
    }
}

void Controller::lcd_drawInfo_Osc_ActiveData(Osc &osc_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        xPos = kInfoStartX + 128;
        yPos = kInfoStartY + 24;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        xPos = kInfoStartX + 356;
        yPos = kInfoStartY + 24;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    const char *textPtr;
    (osc_.active) ? textPtr = kDataShortLOn : textPtr = kDataShortLOff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Osc_WavetableData(Osc &osc_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        xPos = kInfoStartX + 250;
        yPos = kInfoStartY + 24;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        xPos = kInfoStartX + 478;
        yPos = kInfoStartY + 24;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(osc_.wavetableLoadedData.nameShortR, 10, xPos, yPos);
}

void Controller::lcd_drawInfo_Osc_LevelData(Osc &osc_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        xPos = kInfoStartX + 128;
        yPos = kInfoStartY + 104;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        xPos = kInfoStartX + 356;
        yPos = kInfoStartY + 104;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    char kData[5];
    sprintf(kData, "%03d ", osc_.level);
    lcd.drawText(kData, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Osc_TuneData(Osc &osc_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        xPos = kInfoStartX + 292;
        yPos = kInfoStartY + 104;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        xPos = kInfoStartX + 520;
        yPos = kInfoStartY + 104;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kOscTuneDataLibrary[osc_.tune].nameShortR, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Osc_PhaseData(Osc &osc_) {
    RGB16Color foreColor;
    RGB16Color backColor;

    lcd.setForeColor(BLACK);

    for (uint8_t i = 0; i < 4; i++) {
        lcd.drawVLine(kInfoPhaseX0[osc_.number][i], kInfoPhaseY0, 3);
        lcd.drawVLine(kInfoPhaseX0[osc_.number][i], kInfoPhaseY1, 3);
        lcd.drawVLine(kInfoPhaseX1[osc_.number][i], kInfoPhaseY0, 3);
        lcd.drawVLine(kInfoPhaseX1[osc_.number][i], kInfoPhaseY1, 3);
    }

    switch (osc_.number) {
    case 0:
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);

    lcd.drawVLine(kInfoPhaseX0[osc_.number][osc_.phase], kInfoPhaseY0, 3);
    lcd.drawVLine(kInfoPhaseX0[osc_.number][osc_.phase], kInfoPhaseY1, 3);
    lcd.drawVLine(kInfoPhaseX1[osc_.number][osc_.phase], kInfoPhaseY0, 3);
    lcd.drawVLine(kInfoPhaseX1[osc_.number][osc_.phase], kInfoPhaseY1, 3);
}

void Controller::lcd_drawInfo_Osc_GraphData(Osc &osc_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        xPos = kInfoStartX + 128;
        yPos = kInfoStartY + 45;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        xPos = kInfoStartX + 356;
        yPos = kInfoStartY + 45;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);

    if ((osc_.active) && (osc_.wavetableLoaded != -1)) {
        bool flipX = osc_.xFlip;
        bool flipY = osc_.yFlip;
        for (uint8_t i = 0; i < 41; i++) {
            lcd.setForeColor(backColor);
            lcd.drawVLine(xPos, yPos, 45);
            lcd.setForeColor(foreColor);
            uint16_t offset;
            (flipY) ? offset = 4094 - (i * 50 * 2) : offset = i * 50 * 2;
            int16_t data =
                (int16_t)((sdram_read16BitAudio(kRamWavetableAddressLibrary[osc_.number][osc_.playWavetableSector] + (osc_.waveStart * 2048 * 2) + offset) / 32767.0f) * 22);
            (flipX) ? data *= -1 : data = data;
            if (data > 0) {
                lcd.drawVLine(xPos, yPos + (22 - data), data);
            } else if (data < 0) {
                lcd.drawVLine(xPos, yPos + 22, -data);
            } else {
                lcd.drawPixel(xPos, yPos + 22);
            }
            xPos += 2;
        }

        xPos += 27;

        for (uint8_t i = 0; i < 41; i++) {
            lcd.setForeColor(backColor);
            lcd.drawVLine(xPos, yPos, 45);
            lcd.setForeColor(foreColor);
            uint16_t offset;
            (flipY) ? offset = 4094 - (i * 50 * 2) : offset = i * 50 * 2;
            int16_t data =
                (int16_t)((sdram_read16BitAudio(kRamWavetableAddressLibrary[osc_.number][osc_.playWavetableSector] + (osc_.waveEnd * 2048 * 2) + offset) / 32767.0f) * 22);
            (flipX) ? data *= -1 : data = data;
            if (data > 0) {
                lcd.drawVLine(xPos, yPos + (22 - data), data);
            } else if (data < 0) {
                lcd.drawVLine(xPos, yPos + 22, -data);
            } else {
                lcd.drawPixel(xPos, yPos + 22);
            }
            xPos += 2;
        }
    } else {
        for (uint8_t i = 0; i < 41; i++) {
            lcd.setForeColor(backColor);
            lcd.drawVLine(xPos, yPos, 45);
            lcd.setForeColor(foreColor);
            lcd.drawPixel(xPos, yPos + 22);
            xPos += 2;
        }

        xPos += 27;

        for (uint8_t i = 0; i < 41; i++) {
            lcd.setForeColor(backColor);
            lcd.drawVLine(xPos, yPos, 45);
            lcd.setForeColor(foreColor);
            lcd.drawPixel(xPos, yPos + 22);
            xPos += 2;
        }
    }
}

void Controller::lcd_drawInfo_OscLfo_ActiveData(Osc &osc_, Lfo &lfo_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 128;
            break;

        case 1:
            xPos = kInfoStartX + 237;
            break;
        }
        yPos = kInfoStartY + 133;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 356;
            break;

        case 1:
            xPos = kInfoStartX + 465;
            break;
        }
        yPos = kInfoStartY + 133;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    const char *textPtr;
    (lfo_.active) ? textPtr = kDataShortLOn : textPtr = kDataShortLOff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_OscLfo_TypeData(Osc &osc_, Lfo &lfo_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 183;
            break;

        case 1:
            xPos = kInfoStartX + 292;
            break;
        }
        yPos = kInfoStartY + 133;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 411;
            break;

        case 1:
            xPos = kInfoStartX + 520;
            break;
        }
        yPos = kInfoStartY + 133;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kLfoTypeDataLibrary[lfo_.type].nameShortR, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_OscLfo_TargetData(Osc &osc_, Lfo &lfo_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 128;
            break;

        case 1:
            xPos = kInfoStartX + 237;
            break;
        }
        yPos = kInfoStartY + 213;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 356;
            break;

        case 1:
            xPos = kInfoStartX + 465;
            break;
        }
        yPos = kInfoStartY + 213;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kLfoTargetDataLibrary[lfo_.target].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_OscLfo_RateData(Osc &osc_, Lfo &lfo_) {
    uint16_t xPos;
    uint16_t yPos;

    RGB16Color foreColor;
    RGB16Color backColor;

    switch (osc_.number) {
    case 0:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 183;
            break;

        case 1:
            xPos = kInfoStartX + 292;
            break;
        }
        yPos = kInfoStartY + 213;
        foreColor = GREEN;
        backColor = BLACK;
        break;

    case 1:
        switch (lfo_.number) {
        case 0:
            xPos = kInfoStartX + 411;
            break;

        case 1:
            xPos = kInfoStartX + 520;
            break;
        }
        yPos = kInfoStartY + 213;
        foreColor = CYAN;
        backColor = BLACK;
        break;
    }

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kLfoRateDataLibrary[lfo_.rate].nameShortR, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_OscLfo_GraphData(Osc &osc_, Lfo &lfo_) {
    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    switch (osc_.number) {
    case 0:
        indexPtr = (const RGB16Color *)(RAM_GRAPH_LFO_A_PALETTE_ADDRESS);
        (lfo_.active)
            ? dataPtr = (const uint8_t *)(RAM_GRAPH_LFO_A_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * (lfo_.type + 1)))
            : dataPtr = (const uint8_t *)(RAM_GRAPH_LFO_A_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoOscLfoGraphX[osc_.number][lfo_.number], kInfoOscLfoGraphY, kInfoGraphWidth, kInfoGraphHeight);
        break;

    case 1:
        indexPtr = (const RGB16Color *)(RAM_GRAPH_LFO_B_PALETTE_ADDRESS);
        (lfo_.active)
            ? dataPtr = (const uint8_t *)(RAM_GRAPH_LFO_B_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * (lfo_.type + 1)))
            : dataPtr = (const uint8_t *)(RAM_GRAPH_LFO_B_DATA_ADDRESS);
        lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoOscLfoGraphX[osc_.number][lfo_.number], kInfoOscLfoGraphY, kInfoGraphWidth, kInfoGraphHeight);
        break;
    }
}

// info filter

void Controller::lcd_drawInfo_Filter_Select(uint8_t filterNum_, bool selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    palettePtr = (const RGB16Color *)(RAM_BUTTON_FILTER_PALETTE_ADDRESS);
    (selected) ? dataPtr = (const uint8_t *)(RAM_BUTTON_FILTER_ON_DATA_ADDRESS) : dataPtr = (const uint8_t *)(RAM_BUTTON_FILTER_OFF_DATA_ADDRESS);

    lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonFilterX[filterNum_], kButtonFilterY[filterNum_], kButtonFilterWidth, kButtonFilterHeight);
}

void Controller::lcd_drawInfo_Filter_ActiveData(uint8_t filterNum_) {
    uint16_t xPos = kInfoStartX + 584;
    uint16_t yPos;

    switch (filterNum_) {
    case 0:
        yPos = kInfoStartY + 24;
        break;

    case 1:
        yPos = kInfoStartY + 133;
        break;
    }

    RGB16Color foreColor = LBLUE;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    const char *textPtr;
    (filter[filterNum_].active) ? textPtr = kDataShortLOn : textPtr = kDataShortLOff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Filter_TypeData(uint8_t filterNum_) {
    uint16_t xPos = kInfoStartX + 639;
    uint16_t yPos;

    switch (filterNum_) {
    case 0:
        yPos = kInfoStartY + 24;
        break;

    case 1:
        yPos = kInfoStartY + 133;
        break;
    }

    RGB16Color foreColor = LBLUE;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    lcd.drawText(kFilterTypeDataLibrary[filter[filterNum_].type].nameShortR, kMenuDataShortTextSize, xPos, yPos);

    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    if (filter[filterNum_].active) {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_FILTER_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_FILTER_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * filter[filterNum_].type));
    } else {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_FILTER_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_FILTER_DATA_ADDRESS);
    }

    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoFilterGraphX[filterNum_], kInfoFilterGraphY[filterNum_], kInfoGraphWidth, kInfoGraphHeight);
}

void Controller::lcd_drawInfo_Filter_FreqData(uint8_t filterNum_) {
    uint16_t xPos = kInfoStartX + 584;
    uint16_t yPos;

    switch (filterNum_) {
    case 0:
        yPos = kInfoStartY + 104;
        break;

    case 1:
        yPos = kInfoStartY + 213;
        break;
    }

    RGB16Color foreColor = LBLUE;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    lcd.drawText(kFilterFreqDataLibrary[filter[filterNum_].freq].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Filter_ResData(uint8_t filterNum_) {
    uint16_t xPos = kInfoStartX + 639;
    uint16_t yPos;

    switch (filterNum_) {
    case 0:
        yPos = kInfoStartY + 104;
        break;

    case 1:
        yPos = kInfoStartY + 213;
        break;
    }

    RGB16Color foreColor = LBLUE;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    lcd.drawText(kFilterResDataLibrary[filter[filterNum_].res].nameShortR, kMenuDataShortTextSize, xPos, yPos);
}

// info envelope

void Controller::lcd_drawInfo_Envelope_Select(bool selected) {
    const RGB16Color *palettePtr;
    const uint8_t *dataPtr;

    palettePtr = (const RGB16Color *)(RAM_BUTTON_ENVELOPE_PALETTE_ADDRESS);
    (selected)
        ? dataPtr = (const uint8_t *)(RAM_BUTTON_ENVELOPE_ON_DATA_ADDRESS)
        : dataPtr = (const uint8_t *)(RAM_BUTTON_ENVELOPE_OFF_DATA_ADDRESS);

    lcd.drawRGB16Image(palettePtr, dataPtr, 64, kButtonEnvelopeX, kButtonEnvelopeY, kButtonEnvelopeWidth, kButtonEnvelopeHeight);
}

void Controller::lcd_drawInfo_Envelope_ActiveData() {
    uint16_t xPos = kInfoStartX + 703;
    uint16_t yPos = kInfoStartY + 24;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    const char *textPtr;
    (envelope.active) ? textPtr = kDataShortLOn : textPtr = kDataShortLOff;
    lcd.drawText(textPtr, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Envelope_TypeData() {
    uint16_t xPos = kInfoStartX + 758;
    uint16_t yPos = kInfoStartY + 24;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kEnvelopeTypeDataLibrary[envelope.type].nameShortR, kMenuDataShortTextSize, xPos, yPos);

    const RGB16Color *indexPtr;
    const uint8_t *dataPtr;

    if (envelope.active) {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_ENVELOPE_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_ENVELOPE_DATA_ADDRESS + (kInfoGraphWidth * kInfoGraphHeight * envelope.type));
    } else {
        indexPtr = (const RGB16Color *)(RAM_GRAPH_ENVELOPE_PALETTE_ADDRESS);
        dataPtr = (const uint8_t *)(RAM_GRAPH_ENVELOPE_DATA_ADDRESS);
    }

    lcd.drawRGB16Image(indexPtr, dataPtr, 64, kInfoEnvelopeGraphX, kInfoEnvelopeGraphY, kInfoGraphWidth, kInfoGraphHeight);
}

void Controller::lcd_drawInfo_Envelope_CurveData() {
    uint16_t xPos = kInfoStartX + 703;
    uint16_t yPos = kInfoStartY + 104;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);
    lcd.drawText(kEnvelopeCurveDataLibrary[envelope.curve].nameShortL, kMenuDataShortTextSize, xPos, yPos);
}

void Controller::lcd_drawInfo_Envelope_AttackTimeData() {
    uint16_t xPos = kInfoStartX + 758;
    uint16_t yPos = kInfoStartY + 143;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    switch (envelope.type) {
    case ENV_OFF:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, xPos, yPos);
        break;

    case ENV_ADSR:
    case ENV_ASR:
    case ENV_AD:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.attackTime].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;
    }
}

void Controller::lcd_drawInfo_Envelope_DecayTimeData() {
    uint16_t xPos = kInfoStartX + 758;
    uint16_t yPos = kInfoStartY + 163;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    switch (envelope.type) {
    case ENV_OFF:
    case ENV_ASR:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, xPos, yPos);
        break;

    case ENV_ADSR:
    case ENV_AD:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.decayTime].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;
    }
}

void Controller::lcd_drawInfo_Envelope_SustainLevelData() {
    uint16_t xPos = kInfoStartX + 758;
    uint16_t yPos = kInfoStartY + 183;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    switch (envelope.type) {
    case ENV_OFF:
    case ENV_ASR:
    case ENV_AD:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, xPos, yPos);
        break;

    case ENV_ADSR:
        lcd.drawText(kEnvelopeLevelDataLibrary[envelope.sustainLevel].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;
    }
}

void Controller::lcd_drawInfo_Envelope_ReleaseTimeData() {
    uint16_t xPos = kInfoStartX + 758;
    uint16_t yPos = kInfoStartY + 203;

    RGB16Color foreColor = MAGENTA;
    RGB16Color backColor = BLACK;

    lcd.setForeColor(foreColor);
    lcd.setBackColor(backColor);
    lcd.setFont(FONT_05x07);
    lcd.setAlignment(LEFT);

    switch (envelope.type) {
    case ENV_OFF:
    case ENV_AD:
        lcd.drawText(kDataDashShortR, kMenuDataShortTextSize, xPos, yPos);
        break;

    case ENV_ADSR:
    case ENV_ASR:
        lcd.drawText(kEnvelopeTimeDataLibrary[envelope.releaseTime].nameShortR, kMenuDataShortTextSize, xPos, yPos);
        break;
    }
}

/* Main functions ------------------------------------------------------------*/

void Controller::main_select() {
    if (menu != MAIN_MENU) {
        preMenu = menu;
        menu = MAIN_MENU;
        preMenuTab = menuTab;
        menuTab = -1;
        subMenuTab = -1;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawMainMenu();
    }
}

/* File functions ------------------------------------------------------------*/

void Controller::file_select() {
    if (menu != FILE_MENU) {
        preMenu = menu;
        menu = FILE_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFileMenu();
    }
}

void Controller::file_menuRight() {
    if (menuTab < kMaxMenu4Tab) {
        preMenuTab = menuTab;
        menuTab += 1;
        fileMenuCounter = 0;
        sd_checkFile(fileMenuCounter);
        lcd_transitionSelect();

        switch (menuTab) {
        case 1:
            lcd_drawFileBox(1, fileStatus);
            lcd_drawFile_NewData();
            lcd_drawFile_LoadData();
            break;

        case 2:
            lcd_clearFileBox(1);
            lcd_drawFileBox(2, fileStatus);
            lcd_drawFile_LoadData();
            lcd_drawFile_SaveData();
            break;

        case 3:
            lcd_clearFileBox(2);
            lcd_drawFileBox(3, fileStatus);
            lcd_drawFile_SaveData();
            lcd_drawFile_ClearData();
            break;
        }
    }
}

void Controller::file_menuLeft() {
    if (menuTab > kMinMenu4Tab) {
        preMenuTab = menuTab;
        menuTab -= 1;
        fileMenuCounter = 0;
        sd_checkFile(fileMenuCounter);
        lcd_transitionSelect();

        switch (menuTab) {
        case 0:
            lcd_clearFileBox(1);
            lcd_drawFile_NewData();
            lcd_drawFile_LoadData();
            break;

        case 1:
            lcd_clearFileBox(2);
            lcd_drawFileBox(1, fileStatus);
            lcd_drawFile_LoadData();
            lcd_drawFile_SaveData();
            break;

        case 2:
            lcd_clearFileBox(3);
            lcd_drawFileBox(2, fileStatus);
            lcd_drawFile_SaveData();
            lcd_drawFile_ClearData();
            break;
        }
    }
}

void Controller::file_menuUp() {
    (fileMenuCounter < kMaxFile) ? fileMenuCounter += 1 : fileMenuCounter = kMinFile;
    sd_checkFile(fileMenuCounter);

    switch (menuTab) {
    case 0:
        break;

    case 1:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_LoadData();
        break;

    case 2:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_SaveData();
        break;

    case 3:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_ClearData();
        break;
    }
}

void Controller::file_menuDown() {
    (fileMenuCounter > kMinFile) ? fileMenuCounter -= 1 : fileMenuCounter = kMaxFile;
    sd_checkFile(fileMenuCounter);

    switch (menuTab) {
    case 0:
        break;

    case 1:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_LoadData();
        break;

    case 2:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_SaveData();
        break;

    case 3:
        lcd_drawFileBox(menuTab, fileStatus);
        lcd_drawFile_ClearData();
        break;
    }
}

void Controller::file_newSelect() {
    alertFlag = true;
    alertType = ALERT_NEWFILE;
    lcd_drawAlert();
}

void Controller::file_newAction() {
    lcd_clearAlert();

    setOctave(kInitialOctave);
    activeBankNum = 0;
    lcd_drawBank(activeBankNum, false);

    key_reset();
    song_reset();

    rhythm_reset();
    metro_reset();
    osc_reset(osc[0]);
    osc_reset(osc[1]);
    eq_reset();
    filter_reset(0);
    filter_reset(1);
    envelope_reset();
    effect_reset(0);
    effect_reset(1);
    reverb_reset();

    copyBeatNote = -1;
    copyBeatOctave = -1;
    copyBankSongNum = -1;
}

void Controller::file_loadSelect() {
    if (fileStatus == FILE_ACTIVE) {
        alertFlag = true;
        alertType = ALERT_LOADFILE;
        lcd_drawAlert();
    }
}

void Controller::file_loadAction() {
    lcd_clearAlert();

    setOctave(kInitialOctave);
    activeBankNum = 0;
    lcd_drawBank(activeBankNum, false);
    
    song_reset();

    switch (sd_loadFile(fileMenuCounter)) {
    case SD_OK:
        alertType = ALERT_LOADSUCCESS;
        break;

    case SD_ERROR:
        alertType = ALERT_LOADERROR;
        break;

    case SD_ERROR_DETECT:
        alertType = ALERT_MISSINGWAVETABLE;
        break;

    default:
        break;
    }

    copyBeatNote = -1;
    copyBeatOctave = -1;
    copyBankSongNum = -1;

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

void Controller::file_saveSelect() {
    alertFlag = true;
    (fileStatus == FILE_ACTIVE) ? alertType = ALERT_OVERWRITEFILE : alertType = ALERT_SAVEFILE;
    lcd_drawAlert();
}

void Controller::file_saveAction() {
    lcd_clearAlert();
    (sd_saveFile(fileMenuCounter) == SD_OK) ? alertType = ALERT_SAVESUCCESS : alertType = ALERT_SAVEERROR;

    sd_checkFile(fileMenuCounter);
    lcd_drawFileBox(menuTab, fileStatus);

    sd_getFileLibrary();
    lcd_drawSdData();

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

void Controller::file_clearSelect() {
    if (fileStatus != FILE_INACTIVE) {
        alertFlag = true;
        alertType = ALERT_CLEARFILE;
        lcd_drawAlert();
    }
}

void Controller::file_clearAction() {
    lcd_clearAlert();
    (sd_clearFile(fileMenuCounter) == SD_OK) ? alertType = ALERT_CLEARSUCCESS : alertType = ALERT_CLEARERROR;

    sd_checkFile(fileMenuCounter);
    lcd_drawFileBox(menuTab, fileStatus);

    sd_getFileLibrary();
    lcd_drawSdData();

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

/* Synthkit functions --------------------------------------------------------*/

void Controller::synthkit_select() {
    if (menu != SYNTHKIT_MENU) {
        preMenu = menu;
        menu = SYNTHKIT_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawSynthkitMenu();
    }
}

void Controller::synthkit_menuRight() {
    if (menuTab < kMaxMenu4Tab) {
        preMenuTab = menuTab;
        menuTab += 1;
        synthkitMenuCounter = 0;
        sd_checkSynthkit(synthkitMenuCounter);
        lcd_transitionSelect();

        switch (menuTab) {
        case 1:
            lcd_drawFileBox(1, synthkitStatus);
            lcd_drawSynthkit_NewData();
            lcd_drawSynthkit_LoadData();
            break;

        case 2:
            lcd_clearFileBox(1);
            lcd_drawFileBox(2, synthkitStatus);
            lcd_drawSynthkit_LoadData();
            lcd_drawSynthkit_SaveData();
            break;

        case 3:
            lcd_clearFileBox(2);
            lcd_drawFileBox(3, synthkitStatus);
            lcd_drawSynthkit_SaveData();
            lcd_drawSynthkit_ClearData();
            break;
        }
    }
}

void Controller::synthkit_menuLeft() {
    if (menuTab > kMinMenu4Tab) {
        preMenuTab = menuTab;
        menuTab -= 1;
        synthkitMenuCounter = 0;
        sd_checkSynthkit(synthkitMenuCounter);
        lcd_transitionSelect();

        switch (menuTab) {
        case 0:
            lcd_clearFileBox(1);
            lcd_drawSynthkit_NewData();
            lcd_drawSynthkit_LoadData();
            break;

        case 1:
            lcd_clearFileBox(2);
            lcd_drawFileBox(1, synthkitStatus);
            lcd_drawSynthkit_LoadData();
            lcd_drawSynthkit_SaveData();
            break;

        case 2:
            lcd_clearFileBox(3);
            lcd_drawFileBox(2, synthkitStatus);
            lcd_drawSynthkit_SaveData();
            lcd_drawSynthkit_ClearData();
            break;
        }
    }
}

void Controller::synthkit_menuUp() {
    (synthkitMenuCounter < kMaxSynthkit) ? synthkitMenuCounter += 1 : synthkitMenuCounter = kMinFile;
    sd_checkSynthkit(synthkitMenuCounter);

    switch (menuTab) {
    case 0:
        break;

    case 1:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_LoadData();
        break;

    case 2:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_SaveData();
        break;

    case 3:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_ClearData();
        break;
    }
}

void Controller::synthkit_menuDown() {
    (synthkitMenuCounter > kMinSynthkit) ? synthkitMenuCounter -= 1 : synthkitMenuCounter = kMaxFile;
    sd_checkSynthkit(synthkitMenuCounter);

    switch (menuTab) {
    case 0:
        break;

    case 1:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_LoadData();
        break;

    case 2:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_SaveData();
        break;

    case 3:
        lcd_drawFileBox(menuTab, synthkitStatus);
        lcd_drawSynthkit_ClearData();
        break;
    }
}

void Controller::synthkit_newSelect() {
    alertFlag = true;
    alertType = ALERT_NEWSYNTHKIT;
    lcd_drawAlert();
}

void Controller::synthkit_newAction() {
    lcd_clearAlert();

    key_reset();
    osc_reset(osc[0]);
    osc_reset(osc[1]);
    eq_reset();
    filter_reset(0);
    filter_reset(1);
    envelope_reset();
    effect_reset(0);
    effect_reset(1);
    reverb_reset();
}

void Controller::synthkit_loadSelect() {
    if (synthkitStatus == FILE_ACTIVE) {
        alertFlag = true;
        alertType = ALERT_LOADSYNTHKIT;
        lcd_drawAlert();
    }
}

void Controller::synthkit_loadAction() {
    lcd_clearAlert();
    switch (sd_loadSynthkit(0, synthkitMenuCounter)) {
    case SD_OK:
        alertType = ALERT_LOADSUCCESS;
        break;

    case SD_ERROR:
        alertType = ALERT_LOADERROR;
        break;

    case SD_ERROR_DETECT:
        alertType = ALERT_MISSINGWAVETABLE;
        break;

    default:
        break;
    }

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

void Controller::synthkit_saveSelect() {
    alertFlag = true;
    (synthkitStatus == FILE_ACTIVE) ? alertType = ALERT_OVERWRITESYNTHKIT : alertType = ALERT_SAVESYNTHKIT;
    lcd_drawAlert();
}

void Controller::synthkit_saveAction() {
    lcd_clearAlert();
    (sd_saveSynthkit(0, synthkitMenuCounter) == SD_OK) ? alertType = ALERT_SAVESUCCESS : alertType = ALERT_SAVEERROR;

    sd_checkSynthkit(synthkitMenuCounter);
    lcd_drawFileBox(menuTab, synthkitStatus);

    sd_getSynthkitLibrary();
    lcd_drawSdData();

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

void Controller::synthkit_clearSelect() {
    if (synthkitStatus != FILE_INACTIVE) {
        alertFlag = true;
        alertType = ALERT_CLEARSYNTHKIT;
        lcd_drawAlert();
    }
}

void Controller::synthkit_clearAction() {
    lcd_clearAlert();
    (sd_clearSynthkit(0, synthkitMenuCounter) == SD_OK) ? alertType = ALERT_CLEARSUCCESS : alertType = ALERT_CLEARERROR;

    sd_checkSynthkit(synthkitMenuCounter);
    lcd_drawFileBox(menuTab, synthkitStatus);

    sd_getSynthkitLibrary();
    lcd_drawSdData();

    lcd_drawAlert();
    HAL_Delay(1000);
    lcd_clearAlert();
}

/* System functions ----------------------------------------------------------*/

void Controller::system_select() {
    if (menu != SYSTEM_MENU) {
        preMenu = menu;
        menu = SYSTEM_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawSystemMenu();
    }
}

void Controller::system_reset() {
    system_setVolume(kInitialSystemVolume);
    system_setPan(kInitialSystemPan);
    system_setLimiter(kInitialSystemLimiter);
    system_setMidiIn(kInitialSystemMidiIn);
    system_setMidiOut(kInitialSystemMidiOut);
    system_setSyncIn(kInitialSystemSyncIn);
    system_setSyncOut(kInitialSystemSyncOut);
}

void Controller::system_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::system_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::system_menuUp() {
    switch (menuTab) {
    case 0:
        if (system.volume < kMaxSystemVolume) {
            system_setVolume(system.volume + 1);
        }
        break;

    case 2:
        if (system.pan < kMaxSystemPan) {
            system_setPan(system.pan + 1);
        }
        break;

    case 3:
        system_setLimiter(!system.limiter);
        break;

    case 4:
        if (system.midiIn < kMaxSystemMidiIn) {
            system_setMidiIn(system.midiIn + 1);
        }
        break;

    case 5:
        if (system.midiOut < kMaxSystemMidiOut) {
            system_setMidiOut(system.midiOut + 1);
        }
        break;

    case 6:
        if (system.syncIn < kMaxSystemSyncIn) {
            system_setSyncIn(system.syncIn + 1);
        } else {
            system_setSyncIn(kMinSystemSyncIn);
        }
        break;

    case 7:
        if (system.syncOut < kMaxSystemSyncOut) {
            system_setSyncOut(system.syncOut + 1);
        } else {
            system_setSyncOut(kMinSystemSyncOut);
        }
        break;
    }
}

void Controller::system_menuDown() {
    switch (menuTab) {
    case 0:
        if (system.volume > kMinSystemVolume) {
            system_setVolume(system.volume - 1);
        }
        break;

    case 2:
        if (system.pan > kMinSystemPan) {
            system_setPan(system.pan - 1);
        }
        break;

    case 3:
        system_setLimiter(!system.limiter);
        break;

    case 4:
        if (system.midiIn > kMinSystemMidiIn) {
            system_setMidiIn(system.midiIn - 1);
        }
        break;

    case 5:
        if (system.midiOut > kMinSystemMidiOut) {
            system_setMidiOut(system.midiOut - 1);
        }
        break;

    case 6:
        if (system.syncIn > kMinSystemSyncIn) {
            system_setSyncIn(system.syncIn - 1);
        } else {
            system_setSyncIn(kMaxSystemSyncIn);
        }
        break;

    case 7:
        if (system.syncOut > kMinSystemSyncOut) {
            system_setSyncOut(system.syncOut - 1);
        } else {
            system_setSyncOut(kMaxSystemSyncOut);
        }
        break;
    }
}

void Controller::system_setVolume(uint8_t volume_) {
    if ((volume_ >= kMinSystemVolume) && (volume_ <= kMaxSystemVolume)) {
        // update data
        system.volume = volume_;

        float targetVolume = kSystemVolumeDataLibrary[system.volume].data;
        system_volumeTransition(targetVolume);

        // update lcd
        if (menu == SYSTEM_MENU)
            lcd_drawSystem_VolumeData();
    }
}

void Controller::system_setPan(uint8_t pan_) {
    if ((pan_ >= kMinSystemPan) && (pan_ <= kMaxSystemPan)) {
        // update data
        system.pan = pan_;

        float targetVolumeLeft = kSystemVolumeDataLibrary[kSystemPanDataLibrary[system.pan].left].data;
        float targetVolumeRight = kSystemVolumeDataLibrary[kSystemPanDataLibrary[system.pan].right].data;
        system_panTransition(targetVolumeLeft, targetVolumeRight);

        // update lcd
        if (menu == SYSTEM_MENU)
            lcd_drawSystem_PanData();
    }
}

void Controller::system_setLimiter(bool mode_) {
    // uodate data
    system.limiter = mode_;
    // update lcd
    if (menu == SYSTEM_MENU) {
        lcd_drawSystem_LimiterData();
    }
}

void Controller::system_setMidiIn(uint8_t mode_) {
    if ((mode_ >= kMinSystemMidiIn) && (mode_ <= kMaxSystemMidiIn)) {
        // update data
        system.midiIn = mode_;
        system.midi.rxActive = kSystemMidiDataLibrary[system.midiIn].active;
        system.midi.rxChannel = kSystemMidiDataLibrary[system.midiIn].channel;
        // update lcd
        if (menu == SYSTEM_MENU) {
            lcd_drawSystem_MidiInData();
        }
    }
}

void Controller::system_setMidiOut(uint8_t mode_) {
    if ((mode_ >= kMinSystemMidiOut) && (mode_ <= kMaxSystemMidiOut)) {
        // update data
        system.midiOut = mode_;
        system.midi.txActive = kSystemMidiDataLibrary[system.midiOut].active;
        system.midi.txChannel = kSystemMidiDataLibrary[system.midiOut].channel;
        // update lcd
        if (menu == SYSTEM_MENU) {
            lcd_drawSystem_MidiOutData();
        }
    }
}

void Controller::system_setSyncIn(uint8_t mode_) {
    if ((mode_ >= kMinSystemSyncIn) && (mode_ <= kMaxSystemSyncIn)) {
        // update data
        system.syncIn = mode_;
        system.sync.syncInTempo = kSystemSyncInDataLibrary[system.syncIn].tempoTrigger;
        system.sync.syncInPlay = kSystemSyncInDataLibrary[system.syncIn].playTrigger;
        system.sync.syncInBeat = kSystemSyncInDataLibrary[system.syncIn].beatTrigger;
        if (system.syncIn) {
            system.sync.slaveMode = true;
            system.sync.masterMode = false;
        } else if (system.syncOut) {
            system.sync.slaveMode = false;
            system.sync.masterMode = true;
        } else {
            system.sync.slaveMode = false;
            system.sync.masterMode = false;
        }
        // update lcd
        if (menu == SYSTEM_MENU)
            lcd_drawSystem_SyncInData();
    }
}

void Controller::system_setSyncOut(uint8_t mode_) {
    if ((mode_ >= kMinSystemSyncOut) && (mode_ <= kMaxSystemSyncOut)) {
        // update data
        system.syncOut = mode_;
        switch (mode_) {
        case 0: // off
            HAL_GPIO_WritePin(SYNC_OUT_RX_GPIO_Port, SYNC_OUT_RX_Pin, GPIO_PIN_RESET);
            break;
        case 1: // mode-rw
            HAL_GPIO_WritePin(SYNC_OUT_RX_GPIO_Port, SYNC_OUT_RX_Pin, GPIO_PIN_SET);
            break;
        case 2: // mode-st
            HAL_GPIO_WritePin(SYNC_OUT_RX_GPIO_Port, SYNC_OUT_RX_Pin, GPIO_PIN_RESET);
            break;
        }
        system.sync.syncOutTempo = kSystemSyncOutDataLibrary[system.syncOut].tempoTrigger;
        system.sync.syncOutPlay = kSystemSyncOutDataLibrary[system.syncOut].playTrigger;
        system.sync.syncOutBeat = kSystemSyncOutDataLibrary[system.syncOut].beatTrigger;
        if (system.syncIn) {
            system.sync.slaveMode = true;
            system.sync.masterMode = false;
        } else if (system.syncOut) {
            system.sync.slaveMode = false;
            system.sync.masterMode = true;
        } else {
            system.sync.slaveMode = false;
            system.sync.masterMode = false;
        }
        // update lcd
        if (menu == SYSTEM_MENU)
            lcd_drawSystem_SyncOutData();
    }
}

void Controller::system_volumeTransition(float volumeFloat_) {
    SystemVolumeTransition &vTransition = system.volumeTransition;

    vTransition.targetVolume = volumeFloat_;
    system_calculateVolumeTransition();
    vTransition.active = true;
}

void Controller::system_panTransition(float volumeLeftFloat_, float volumeRightFloat_) {
    SystemPanTransition &pTransition = system.panTransition;

    pTransition.targetVolumeLeft = volumeLeftFloat_;
    pTransition.targetVolumeRight = volumeRightFloat_;

    system_calculatePanTransition();
    pTransition.active = true;
}

void Controller::system_calculateVolumeTransition() {
    SystemVolumeTransition &vTransition = system.volumeTransition;

    if (system.volumeFloat == vTransition.targetVolume) {
        vTransition.actionVolume = SYS_ACTION_NONE;
    } else if (system.volumeFloat < vTransition.targetVolume) {
        vTransition.actionVolume = SYS_ACTION_UP;
    } else if (system.volumeFloat > vTransition.targetVolume) {
        vTransition.actionVolume = SYS_ACTION_DOWN;
    }
}

void Controller::system_calculatePanTransition() {
    SystemPanTransition &pTransition = system.panTransition;

    if (system.volumeLeftFloat == pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = SYS_ACTION_NONE;
    } else if (system.volumeLeftFloat < pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = SYS_ACTION_UP;
    } else if (system.volumeLeftFloat > pTransition.targetVolumeLeft) {
        pTransition.actionVolumeLeft = SYS_ACTION_DOWN;
    }

    if (system.volumeRightFloat == pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = SYS_ACTION_NONE;
    } else if (system.volumeRightFloat < pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = SYS_ACTION_UP;
    } else if (system.volumeRightFloat > pTransition.targetVolumeRight) {
        pTransition.actionVolumeRight = SYS_ACTION_DOWN;
    }
}

/* Rhythm functions ----------------------------------------------------------*/

void Controller::preMenuSongClear() {
    if ((preMenu == SONG_MENU) &&
        (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)) {
        lcd_drawBeat(activeBankNum, selectedBeatNum, false);
        selectedBeatNum = -1;
    }

    if (preMenu == SONG_MENU) {
        if ((song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) && (selectedBeatNum != -1))
            lcd_drawBeat(activeBankNum, selectedBeatNum, false);
        selectedBeatNum = -1;
    }
}

void Controller::rhythm_select() {
    if (menu != RHYTHM_MENU) {
        preMenu = menu;
        menu = RHYTHM_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawRhythmMenu();
    }

    // if (!wavReadFlag) wavReadFlag = true;  // wavTest
}

void Controller::rhythm_reset() {
    rhythm_setTempo(kInitialTempo);
    rhythm_setMeasure(kInitialMeasure);
    rhythm_setBar(kInitialBar);
    rhythm_setQuantize(kInitialQuantize);
}

void Controller::rhythm_menuRight() {
    if (menuTab < kMaxMenu4Tab) {
        preMenuTab = menuTab;
        menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::rhythm_menuLeft() {
    if (menuTab > kMinMenu4Tab) {
        preMenuTab = menuTab;
        menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::rhythm_menuUp() {
    switch (menuTab) {
    case 0:
        if ((rhythm.tempo < kMaxTempo) && (!system.sync.slaveMode)) {
            rhythm_setTempo(rhythm.tempo + 1);
        }
        break;

    case 1:
        if (!rhythm.measureLock) {
            if (rhythm.measure < kMaxMeasure) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_MEASUREUP;
                    lcd_drawAlert();
                } else {
                    rhythm_setMeasure(rhythm.measure + 1);
                }
            }
        }
        break;

    case 2:
        if (!rhythm.barLock) {
            if (rhythm.bar < kMaxBar) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_BARUP;
                    lcd_drawAlert();
                } else {
                    rhythm_setBar(rhythm.bar + 1);
                }
            }
        }
        break;

    case 3:
        if (!rhythm.quantizeLock) {
            if (rhythm.quantize < kMaxQuantize) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_QUANTIZEUP;
                    lcd_drawAlert();
                } else {
                    rhythm_setQuantize(rhythm.quantize + 1);
                }
            }
        }
        break;
    }
}

void Controller::rhythm_menuDown() {
    switch (menuTab) {
    case 0:
        if ((rhythm.tempo > kMinTempo) && (!system.sync.slaveMode)) {
            rhythm_setTempo(rhythm.tempo - 1);
        }
        break;

    case 1:
        if (!rhythm.measureLock) {
            if (rhythm.measure > kMinMeasure) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_MEASUREDOWN;
                    lcd_drawAlert();
                } else {
                    rhythm_setMeasure(rhythm.measure - 1);
                }
            }
        }
        break;

    case 2:
        if (!rhythm.barLock) {
            if (rhythm.bar > kMinBar) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_BARDOWN;
                    lcd_drawAlert();
                } else {
                    rhythm_setBar(rhythm.bar - 1);
                }
            }
        }
        break;

    case 3:
        if (!rhythm.quantizeLock) {
            if (rhythm.quantize > kMinQuantize) {
                if (playActive) {
                    alertFlag = true;
                    alertType = ALERT_QUANTIZEDOWN;
                    lcd_drawAlert();
                } else {
                    rhythm_setQuantize(rhythm.quantize - 1);
                }
            }
        }
        break;
    }
}

void Controller::rhythm_setTempo(uint8_t tempo_) {
    if ((tempo_ >= kMinTempo) && (tempo_ <= kMaxTempo)) {
        // sync data
        if (system.sync.syncOutTempo)
            sendSyncCommand(tempo_);
        // update data
        rhythm.tempo = tempo_;
        updatePlayTimerPeriod();
        effect[0].delay.update(tempo_);
        effect[1].delay.update(tempo_);
        // update lcd
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_TempoData();
        if (menu == MAIN_MENU)
            lcd_drawMain_TempoData();
    }
}

void Controller::rhythm_setMeasure(uint8_t measure_) {
    if ((measure_ >= kMinMeasure) && (measure_ <= kMaxMeasure)) {
        // update data
        rhythm.measure = measure_;
        calculateSongInterval();
        adjustMeasureBarTiming();
        // update lcd
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_MeasureData();
        if (menu == MAIN_MENU)
            lcd_drawMain_MeasureData();
        // update song
        lcd_calculateSongX();
        lcd_resetPlay();
        lcd_drawMeasureBar();
        lcd_drawSong(activeBankNum);
        reset();
    }
}

void Controller::rhythm_setBar(uint8_t bar_) {
    if ((bar_ >= kMinBar) && (bar_ <= kMaxBar)) {
        // update data
        rhythm.bar = bar_;
        calculateSongInterval();
        adjustMeasureBarTiming();
        // update lcd
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_BarData();
        if (menu == MAIN_MENU)
            lcd_drawMain_BarData();
        // update song
        lcd_calculateSongX();
        lcd_resetPlay();
        lcd_drawMeasureBar();
        lcd_drawSong(activeBankNum);
        reset();
    }
}

void Controller::rhythm_setQuantize(uint8_t quantize_) {
    if ((quantize_ >= kMinQuantize) && (quantize_ <= kMaxQuantize)) {
        // update data
        rhythm.quantize = quantize_;
        // update lcd
        if (menu == RHYTHM_MENU)
            lcd_drawRhythm_QuantizeData();
        if (menu == MAIN_MENU)
            lcd_drawMain_QuantizeData();
        // quantize beats
        for (uint8_t i = 0; i < kBankLibrarySize; i++) {
            song_quantizeActiveBeats(i);
        }
        // update song
        lcd_drawSong(activeBankNum);
        reset();
    }
}

/* Metronome functions -------------------------------------------------------*/

void Controller::metro_select() {
    if (menu != METRO_MENU) {
        preMenu = menu;
        menu = METRO_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawMetroMenu();
    }
}

void Controller::metro_reset() {
    metro_setActive(kInitialMetroActive);
    metro_setPrecount(kInitialMetroPrecount);
    metro_setSample(kInitialMetroSample);
    metro_setVolume(kInitialMetroVolume);
}

void Controller::metro_menuRight() {
    if (menuTab < kMaxMenu4Tab) {
        preMenuTab = menuTab;
        menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::metro_menuLeft() {
    if (menuTab > kMinMenu4Tab) {
        preMenuTab = menuTab;
        menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::metro_menuUp() {
    switch (menuTab) {
    case 0:
        metro_setActive(!metronome.active);
        break;

    case 1:
        metro_setPrecount(!metronome.precount);
        break;

    case 2:
        (metronome.sample < kMaxMetroSample) ? metro_setSample(metronome.sample + 1) : metro_setSample(kMinMetroSample);
        break;

    case 3:
        if (metronome.volume < kMaxMetroVolume) {
            metro_setVolume(metronome.volume + 1);
        }
        break;
    }
}

void Controller::metro_menuDown() {
    switch (menuTab) {
    case 0:
        metro_setActive(!metronome.active);
        break;

    case 1:
        metro_setPrecount(!metronome.precount);
        break;

    case 2:
        (metronome.sample > kMinMetroSample) ? metro_setSample(metronome.sample - 1) : metro_setSample(kMaxMetroSample);
        break;

    case 3:
        if (metronome.volume > kMinMetroVolume) {
            metro_setVolume(metronome.volume - 1);
        }
        break;
    }
}

void Controller::metro_setActive(bool active_) {
    // update data
    metronome.active = active_;
    // update lcd
    if (menu == METRO_MENU)
        lcd_drawMetro_ActiveData();
}

void Controller::metro_setPrecount(bool precount_) {
    // update data
    metronome.precount = precount_;
    calculateSongInterval();
    // update lcd
    if (menu == METRO_MENU)
        lcd_drawMetro_PrecountData();
}

void Controller::metro_setSample(uint8_t sample_) {
    if ((sample_ >= kMinMetroSample) && (sample_ <= kMaxMetroSample)) {
        // update data
        metronome.sample = sample_;
        // update lcd
        if (menu == METRO_MENU)
            lcd_drawMetro_SampleData();
    }
}

void Controller::metro_setVolume(uint8_t volume_) {
    if ((volume_ >= kMinMetroVolume) && (volume_ <= kMaxMetroVolume)) {
        // update data
        metronome.volume = volume_;

        float targetVolume = kMetronomeVolumeDataLibrary[metronome.volume].data;
        metro_volumeTransition(targetVolume);

        // update lcd
        if (menu == METRO_MENU)
            lcd_drawMetro_VolumeData();
    }
}

void Controller::metro_volumeTransition(float volumeFloat_) {
    MetronomeVolumeTransition &vTransition = metronome.volumeTransition;

    vTransition.targetVolume = volumeFloat_;
    metro_calculateVolumeTransition();
    vTransition.active = true;
}

void Controller::metro_calculateVolumeTransition() {
    MetronomeVolumeTransition &vTransition = metronome.volumeTransition;

    if (metronome.volumeFloat == vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_NONE;
    } else if (metronome.volumeFloat < vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_UP;
    } else if (metronome.volumeFloat > vTransition.targetVolume) {
        vTransition.actionVolume = MET_ACTION_DOWN;
    }
}

/* Eq functions --------------------------------------------------------------*/

void Controller::eq_select() {
    if (menu != EQ_MENU) {
        preMenu = menu;
        menu = EQ_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEqMenu();
    }
}

void Controller::eq_reset() {
    eq_setActive(kInitialEqActive);
    eq_setFreqLowShelf(kInitialEqFreqLowShelf);
    eq_setGainLowShelf(kInitialEqGainLowShelf);
    eq_setFreqHighShelf(kInitialEqFreqHighShelf);
    eq_setGainHighShelf(kInitialEqGainHighShelf);
    for (uint8_t i = 0; i < 4; i++) {
        eq_setFreqPeak(i, kInitialEqFreqPeak[i]);
        eq_setGainPeak(i, kInitialEqGainPeak[i]);
        eq_setQPeak(i, kInitialEqQPeak[i]);
    }
}

void Controller::eq_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        if ((menuTab == 6) && (subMenuTab == 2))
            subMenuTab = 1;
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::eq_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        if (menuTab == 2)
            subMenuTab = 0;
        if ((menuTab == 3) && (subMenuTab == 2))
            subMenuTab = 1;
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::eq_menuUp() {
    switch (menuTab) {
    case 0:
        eq_setActive(!eq.active);
        break;

    case 2:
        switch (subMenuTab) {
        case 0:
            if (eq.gainLowShelf < kMaxEqGain) {
                eq_setGainLowShelf(eq.gainLowShelf + 1);
            }
            break;

        case 1:
            if (eq.freqLowShelf < kMaxEqFreq) {
                eq_setFreqLowShelf(eq.freqLowShelf + 1);
            }
            break;
        }
        break;

    case 3:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[0] < kMaxEqGain) {
                eq_setGainPeak(0, eq.gainPeak[0] + 1);
            }
            break;

        case 1:
            if (eq.freqPeak[0] < kMaxEqFreq) {
                eq_setFreqPeak(0, eq.freqPeak[0] + 1);
            }
            break;

        case 2:
            if (eq.qPeak[0] < kMaxEqQ) {
                eq_setQPeak(0, eq.qPeak[0] + 1);
            }
            break;
        }
        break;

    case 4:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[1] < kMaxEqGain) {
                eq_setGainPeak(1, eq.gainPeak[1] + 1);
            }
            break;

        case 1:
            if (eq.freqPeak[1] < kMaxEqFreq) {
                eq_setFreqPeak(1, eq.freqPeak[1] + 1);
            }
            break;

        case 2:
            if (eq.qPeak[1] < kMaxEqQ) {
                eq_setQPeak(1, eq.qPeak[1] + 1);
            }
            break;
        }
        break;

    case 5:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[2] < kMaxEqGain) {
                eq_setGainPeak(2, eq.gainPeak[2] + 1);
            }
            break;

        case 1:
            if (eq.freqPeak[2] < kMaxEqFreq) {
                eq_setFreqPeak(2, eq.freqPeak[2] + 1);
            }
            break;

        case 2:
            if (eq.qPeak[2] < kMaxEqQ) {
                eq_setQPeak(2, eq.qPeak[2] + 1);
            }
            break;
        }
        break;

    case 6:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[3] < kMaxEqGain) {
                eq_setGainPeak(3, eq.gainPeak[3] + 1);
            }
            break;

        case 1:
            if (eq.freqPeak[3] < kMaxEqFreq) {
                eq_setFreqPeak(3, eq.freqPeak[3] + 1);
            }
            break;

        case 2:
            if (eq.qPeak[3] < kMaxEqQ) {
                eq_setQPeak(3, eq.qPeak[3] + 1);
            }
            break;
        }
        break;

    case 7:
        switch (subMenuTab) {
        case 0:
            if (eq.gainHighShelf < kMaxEqGain) {
                eq_setGainHighShelf(eq.gainHighShelf + 1);
            }
            break;

        case 1:
            if (eq.freqHighShelf < kMaxEqFreq) {
                eq_setFreqHighShelf(eq.freqHighShelf + 1);
            }
            break;
        }
        break;
    }
}

void Controller::eq_menuDown() {
    switch (menuTab) {
    case 0:
        eq_setActive(!eq.active);
        break;

    case 2:
        switch (subMenuTab) {
        case 0:
            if (eq.gainLowShelf > kMinEqGain) {
                eq_setGainLowShelf(eq.gainLowShelf - 1);
            }
            break;

        case 1:
            if (eq.freqLowShelf > kMinEqFreq) {
                eq_setFreqLowShelf(eq.freqLowShelf - 1);
            }
            break;
        }
        break;

    case 3:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[0] > kMinEqGain) {
                eq_setGainPeak(0, eq.gainPeak[0] - 1);
            }
            break;

        case 1:
            if (eq.freqPeak[0] > kMinEqFreq) {
                eq_setFreqPeak(0, eq.freqPeak[0] - 1);
            }
            break;

        case 2:
            if (eq.qPeak[0] > kMinEqQ) {
                eq_setQPeak(0, eq.qPeak[0] - 1);
            }
            break;
        }
        break;

    case 4:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[1] > kMinEqGain) {
                eq_setGainPeak(1, eq.gainPeak[1] - 1);
            }
            break;

        case 1:
            if (eq.freqPeak[1] > kMinEqFreq) {
                eq_setFreqPeak(1, eq.freqPeak[1] - 1);
            }
            break;

        case 2:
            if (eq.qPeak[1] > kMinEqQ) {
                eq_setQPeak(1, eq.qPeak[1] - 1);
            }
            break;
        }
        break;

    case 5:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[2] > kMinEqGain) {
                eq_setGainPeak(2, eq.gainPeak[2] - 1);
            }
            break;

        case 1:
            if (eq.freqPeak[2] > kMinEqFreq) {
                eq_setFreqPeak(2, eq.freqPeak[2] - 1);
            }
            break;

        case 2:
            if (eq.qPeak[2] > kMinEqQ) {
                eq_setQPeak(2, eq.qPeak[2] - 1);
            }
            break;
        }
        break;

    case 6:
        switch (subMenuTab) {
        case 0:
            if (eq.gainPeak[3] > kMinEqGain) {
                eq_setGainPeak(3, eq.gainPeak[3] - 1);
            }
            break;

        case 1:
            if (eq.freqPeak[3] > kMinEqFreq) {
                eq_setFreqPeak(3, eq.freqPeak[3] - 1);
            }
            break;

        case 2:
            if (eq.qPeak[3] > kMinEqQ) {
                eq_setQPeak(3, eq.qPeak[3] - 1);
            }
            break;
        }
        break;

    case 7:
        switch (subMenuTab) {
        case 0:
            if (eq.gainHighShelf > kMinEqGain) {
                eq_setGainHighShelf(eq.gainHighShelf - 1);
            }
            break;

        case 1:
            if (eq.freqHighShelf > kMinEqFreq) {
                eq_setFreqHighShelf(eq.freqHighShelf - 1);
            }
        }
        break;
    }
}

void Controller::eq_setActive(bool active_) {
    if ((menu == EQ_MENU) && (!eq.genTransition.active)) {
        // update data
        // if (active_) eq.cleanMemory();
        eq_genTransition(EQ_MODE_ACTIVE, eq.active, active_);
        eq.active = active_;
        // update lcd
        lcd_drawEq_ActiveData();
    } else if (!eq.genTransition.active) {
        // update data
        eq.active = active_;
    }
}

void Controller::eq_setFreqLowShelf(uint8_t freq_) {
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // update data
        eq.freqLowShelf = freq_;
        eq.calculateCoefLowShelf();
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_LowShelfData();
    }
}

void Controller::eq_setGainLowShelf(uint8_t gain_) {
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // update data
        eq.gainLowShelf = gain_;
        eq.calculateCoefLowShelf();
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_LowShelfData();
    }
}

void Controller::eq_setFreqHighShelf(uint8_t freq_) {
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // update data
        eq.freqHighShelf = freq_;
        eq.calculateCoefHighShelf();
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_HighShelfData();
    }
}

void Controller::eq_setGainHighShelf(uint8_t gain_) {
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // update data
        eq.gainHighShelf = gain_;
        eq.calculateCoefHighShelf();
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_HighShelfData();
    }
}

void Controller::eq_setFreqPeak(uint8_t peakNum_, uint8_t freq_) {
    if ((freq_ >= kMinEqFreq) && (freq_ <= kMaxEqFreq)) {
        // update data
        eq.freqPeak[peakNum_] = freq_;
        eq.calculateCoefPeak(peakNum_);
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_PeakData(peakNum_);
    }
}

void Controller::eq_setGainPeak(uint8_t peakNum_, uint8_t gain_) {
    if ((gain_ >= kMinEqGain) && (gain_ <= kMaxEqGain)) {
        // update data
        eq.gainPeak[peakNum_] = gain_;
        eq.calculateCoefPeak(peakNum_);
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_PeakData(peakNum_);
    }
}

void Controller::eq_setQPeak(uint8_t peakNum_, uint8_t q_) {
    if ((q_ >= kMinEqQ) && (q_ <= kMaxEqQ)) {
        // update data
        eq.qPeak[peakNum_] = q_;
        eq.calculateCoefPeak(peakNum_);
        // update lcd
        if (menu == EQ_MENU)
            lcd_drawEq_PeakData(peakNum_);
    }
}

void Controller::eq_genTransition(EqTransitionMode mode_, bool activeActive_, bool targetActive_) {
    EqGenTransition &gTransition = eq.genTransition;

    gTransition.mode = mode_;
    gTransition.phase = EQ_PHASE_A;

    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    switch (mode_) {
    case EQ_MODE_NONE:
        break;

    case EQ_MODE_ACTIVE:
        switch (targetActive_) {
        case true:
            gTransition.activeDry = 1.0;
            gTransition.targetDry = 0.0;

            gTransition.activeWet = 0.0;
            gTransition.targetWet = 1.0;

            transitionShowFlag = 2;
            break;

        case false:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;

            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;

            transitionShowFlag = 1;
            break;
        }
        break;
    }

    eq_calculateActiveTransition();
    gTransition.active = true;
}

void Controller::eq_calculateActiveTransition() {
    EqGenTransition &gTransition = eq.genTransition;

    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = EQ_ACTION_DOWN;
    }

    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = EQ_ACTION_DOWN;
    }
}

/* Osc functions -------------------------------------------------------------*/

void Controller::osc_select(Osc &osc_) {
    if (osc_.number == 0) {
        if ((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) {
            preMenu = menu;
            menu = OSC_A2_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;

            osc_.wavetableSelected = osc_.wavetableLoaded;
            osc_.wavetableSelectedData = osc_.wavetableLoadedData;
            osc_.wavSelectedData = osc_.wavLoadedData;
            osc_.wavetableSelectedReadError = false;
            osc_.wavetableSelectedTypeError = false;

            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOscLfoMenu(osc[0], osc[0].lfo[0]);
        } else if (menu == OSC_A2_MENU) {
            preMenu = menu;
            menu = OSC_A3_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOscLfoMenu(osc[0], osc[0].lfo[1]);
        } else if (menu == OSC_A3_MENU) {
            preMenu = menu;
            menu = OSC_A0_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOsc0Menu(osc[0]);
        } else {
            preMenu = menu;
            menu = OSC_A0_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;

            osc_.wavetableSelected = osc_.wavetableLoaded;
            osc_.wavetableSelectedData = osc_.wavetableLoadedData;
            osc_.wavSelectedData = osc_.wavLoadedData;
            osc_.wavetableSelectedReadError = false;
            osc_.wavetableSelectedTypeError = false;

            preMenuSongClear();
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOsc0Menu(osc[0]);
        }
    } else if (osc_.number == 1) {
        if ((menu == OSC_B0_MENU) || (menu == OSC_B1_MENU)) {
            preMenu = menu;
            menu = OSC_B2_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;

            osc_.wavetableSelected = osc_.wavetableLoaded;
            osc_.wavetableSelectedData = osc_.wavetableLoadedData;
            osc_.wavSelectedData = osc_.wavLoadedData;
            osc_.wavetableSelectedReadError = false;
            osc_.wavetableSelectedTypeError = false;

            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOscLfoMenu(osc[1], osc[1].lfo[0]);
        } else if (menu == OSC_B2_MENU) {
            preMenu = menu;
            menu = OSC_B3_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOscLfoMenu(osc[1], osc[1].lfo[1]);
        } else if (menu == OSC_B3_MENU) {
            preMenu = menu;
            menu = OSC_B0_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOsc0Menu(osc[1]);
        } else {
            preMenu = menu;
            menu = OSC_B0_MENU;
            preMenuTab = menuTab;
            menuTab = 0;
            subMenuTab = 0;

            osc_.wavetableSelected = osc_.wavetableLoaded;
            osc_.wavetableSelectedData = osc_.wavetableLoadedData;
            osc_.wavSelectedData = osc_.wavLoadedData;
            osc_.wavetableSelectedReadError = false;
            osc_.wavetableSelectedTypeError = false;

            preMenuSongClear();
            lcd_transitionMenu();
            lcd_transitionSelect();
            lcd_drawOsc0Menu(osc[1]);
        }
    }
}

void Controller::osc_reset(Osc &osc_) {
    osc_setActive(osc_, kInitialOscActive);
    osc_setWavetableSelected(osc_, kInitialOscWavetable);
    osc_setWavetableLoaded(osc_, true);
    osc_setTune(osc_, kInitialOscTune);
    osc_setLevel(osc_, kInitialOscLevel);
    osc_setPhase(osc_, kInitialOscPhase);
    osc_setXFlip(osc_, kInitialOscXFlip);
    osc_setYFlip(osc_, kInitialOscYFlip);

    osc_lfo_reset(osc_, osc_.lfo[0]);
    osc_lfo_reset(osc_, osc_.lfo[1]);
}

void Controller::osc_lfo_reset(Osc &osc_, Lfo &lfo_) {
    osc_lfo_setActive(osc_, lfo_, kInitialLfoActive);
    osc_lfo_setType(osc_, lfo_, kInitialLfoType);
    osc_lfo_setRate(osc_, lfo_, kInitialLfoRate);
    osc_lfo_setDepth(osc_, lfo_, kInitialLfoDepth);
}

void Controller::osc_menuRight(Osc &osc_) {
    if ((menu == OSC_A0_MENU) && (menuTab == 7)) {
        preMenu = menu;
        menu = OSC_A1_MENU;
        preMenuTab = menuTab;
        menuTab = 4;
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawOsc1Menu(osc[0]);
    } else if ((menu == OSC_B0_MENU) && (menuTab == 7)) {
        preMenu = menu;
        menu = OSC_B1_MENU;
        preMenuTab = menuTab;
        menuTab = 4;
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawOsc1Menu(osc[1]);
    } else if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        ((menuTab == 0) || (menuTab == 2)) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::osc_menuLeft(Osc &osc_) {
    if ((menu == OSC_A1_MENU) && (menuTab == 4)) {
        preMenu = menu;
        menu = OSC_A0_MENU;
        preMenuTab = menuTab;
        menuTab = 7;
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawOsc0Menu(osc[0]);
    } else if ((menu == OSC_B1_MENU) && (menuTab == 4)) {
        preMenu = menu;
        menu = OSC_B0_MENU;
        preMenuTab = menuTab;
        menuTab = 7;
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawOsc0Menu(osc[1]);
    } else if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        ((menuTab == 2) || (menuTab == 4)) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::osc_menuUp(Osc &osc_) {
    uint8_t lfoTarget;

    switch (menu) {
    case OSC_A0_MENU:
    case OSC_B0_MENU:
        switch (menuTab) {
        case 0:
            osc_setActive(osc_, !osc_.active);
            break;

        case 2:
            if (wavetableLibrarySize > 0) {
                (osc_.wavetableSelected < (wavetableLibrarySize - 1)) ? osc_setWavetableSelected(osc_, osc_.wavetableSelected + 1) : osc_setWavetableSelected(osc_, -1);
            }
            break;

        case 4:
            if (osc_.level < kMaxOscLevel)
                osc_setLevel(osc_, osc_.level + 5);
            break;

        case 5:
            if (osc_.tune < kMaxOscTune)
                osc_setTune(osc_, osc_.tune + 1);
            break;

        case 6:
            if (osc_.phase < kMaxOscPhase)
                osc_setPhase(osc_, osc_.phase + 1);
            break;

        case 7:
            osc_setNormalize(osc_, !osc_.normalize);
            break;
        }
        break;

    case OSC_A1_MENU:
    case OSC_B1_MENU:
        switch (menuTab) {
        case 0:
            osc_setActive(osc_, !osc_.active);
            break;

        case 2:
            if (wavetableLibrarySize > 0) {
                (osc_.wavetableSelected < (wavetableLibrarySize - 1)) ? osc_setWavetableSelected(osc_, osc_.wavetableSelected + 1) : osc_setWavetableSelected(osc_, -1);
            }
            break;

        case 4:
            if (osc_.start < osc_.end)
                osc_setStart(osc_, osc_.start + 1);
            break;

        case 5:
            if (osc_.end < kStepDataLibrary[osc_.wavetableLoadedData.step].max)
                osc_setEnd(osc_, osc_.end + 1);
            break;

        case 6:
            osc_setXFlip(osc_, !osc_.xFlip);
            break;

        case 7:
            osc_setYFlip(osc_, !osc_.yFlip);
            break;
        }
        break;

    case OSC_A2_MENU:
    case OSC_B2_MENU:
        switch (menuTab) {
        case 0:
            osc_lfo_setActive(osc_, osc_.lfo[0], !osc_.lfo[0].active);
            break;

        case 2:
            (osc_.lfo[0].type < kMaxLfoType) ? osc_lfo_setType(osc_, osc_.lfo[0], osc_.lfo[0].type + 1) : osc_lfo_setType(osc_, osc_.lfo[0], kMinLfoType);
            break;

        case 4:
            switch (osc_.lfo[0].target) {
            case 0:
                switch (osc_.lfo[1].target) {
                case 1:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 1;
                    break;
                }
                break;

            case 1:
                switch (osc_.lfo[1].target) {
                case 0:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 0;
                    break;
                }
                break;

            case 2:
                switch (osc_.lfo[1].target) {
                case 0:
                    lfoTarget = 1;
                    break;

                case 1:
                    lfoTarget = 0;
                    break;
                }
                break;
            }

            osc_lfo_setTarget(osc_, osc_.lfo[0], lfoTarget);
            break;

        case 5:
            if (osc_.lfo[0].rate < kMaxLfoRate)
                osc_lfo_setRate(osc_, osc_.lfo[0], osc_.lfo[0].rate + 1);
            break;

        case 6:
            if (osc_.lfo[0].depth < kMaxLfoDepth)
                osc_lfo_setDepth(osc_, osc_.lfo[0], osc_.lfo[0].depth + 1);
            break;

        case 7:
            osc_lfo_setLoop(osc_, osc_.lfo[0], !osc_.lfo[0].loop);
            break;
        }
        break;

    case OSC_A3_MENU:
    case OSC_B3_MENU:
        switch (menuTab) {
        case 0:
            osc_lfo_setActive(osc_, osc_.lfo[1], !osc_.lfo[1].active);
            break;

        case 2:
            (osc_.lfo[1].type < kMaxLfoType) ? osc_lfo_setType(osc_, osc_.lfo[1], osc_.lfo[1].type + 1) : osc_lfo_setType(osc_, osc_.lfo[1], kMinLfoType);
            break;

        case 4:
            switch (osc_.lfo[1].target) {
            case 0:
                switch (osc_.lfo[0].target) {
                case 1:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 1;
                    break;
                }
                break;

            case 1:
                switch (osc_.lfo[0].target) {
                case 0:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 0;
                    break;
                }
                break;

            case 2:
                switch (osc_.lfo[0].target) {
                case 0:
                    lfoTarget = 1;
                    break;

                case 1:
                    lfoTarget = 0;
                    break;
                }
                break;
            }

            osc_lfo_setTarget(osc_, osc_.lfo[1], lfoTarget);

            break;

        case 5:
            if (osc_.lfo[1].rate < kMaxLfoRate)
                osc_lfo_setRate(osc_, osc_.lfo[1], osc_.lfo[1].rate + 1);
            break;

        case 6:
            if (osc_.lfo[1].depth < kMaxLfoDepth)
                osc_lfo_setDepth(osc_, osc_.lfo[1], osc_.lfo[1].depth + 1);
            break;

        case 7:
            osc_lfo_setLoop(osc_, osc_.lfo[1], !osc_.lfo[1].loop);
            break;
        }
        break;

    default:
        break;
    }
}

void Controller::osc_menuDown(Osc &osc_) {
    uint8_t lfoTarget;

    switch (menu) {
    case OSC_A0_MENU:
    case OSC_B0_MENU:
        switch (menuTab) {
        case 0:
            osc_setActive(osc_, !osc_.active);
            break;

        case 2:
            if (wavetableLibrarySize > 0) {
                (osc_.wavetableSelected > -1) ? osc_setWavetableSelected(osc_, osc_.wavetableSelected - 1) : osc_setWavetableSelected(osc_, wavetableLibrarySize - 1);
            }
            break;

        case 4:
            if (osc_.level > kMinOscLevel)
                osc_setLevel(osc_, osc_.level - 5);
            break;

        case 5:
            if (osc_.tune > kMinOscTune)
                osc_setTune(osc_, osc_.tune - 1);
            break;

        case 6:
            if (osc_.phase > kMinOscPhase)
                osc_setPhase(osc_, osc_.phase - 1);
            break;

        case 7:
            osc_setNormalize(osc_, !osc_.normalize);
            break;
        }
        break;

    case OSC_A1_MENU:
    case OSC_B1_MENU:
        switch (menuTab) {
        case 0:
            osc_setActive(osc_, !osc_.active);
            break;

        case 4:
            if (osc_.start > kStepDataLibrary[osc_.wavetableLoadedData.step].min)
                osc_setStart(osc_, osc_.start - 1);
            break;

        case 5:
            if (osc_.end > osc_.start)
                osc_setEnd(osc_, osc_.end - 1);
            break;

        case 6:
            osc_setXFlip(osc_, !osc_.xFlip);
            break;

        case 7:
            osc_setYFlip(osc_, !osc_.yFlip);
            break;
        }
        break;

    case OSC_A2_MENU:
    case OSC_B2_MENU:
        switch (menuTab) {
        case 0:
            osc_lfo_setActive(osc_, osc_.lfo[0], !osc_.lfo[0].active);
            break;

        case 2:
            (osc_.lfo[0].type > kMinLfoType) ? osc_lfo_setType(osc_, osc_.lfo[0], osc_.lfo[0].type - 1) : osc_lfo_setType(osc_, osc_.lfo[0], kMaxLfoType);
            break;

        case 4:
            switch (osc_.lfo[0].target) {
            case 0:
                switch (osc_.lfo[1].target) {
                case 1:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 1;
                    break;
                }
                break;

            case 1:
                switch (osc_.lfo[1].target) {
                case 0:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 0;
                    break;
                }
                break;

            case 2:
                switch (osc_.lfo[1].target) {
                case 0:
                    lfoTarget = 1;
                    break;

                case 1:
                    lfoTarget = 0;
                    break;
                }
                break;
            }

            osc_lfo_setTarget(osc_, osc_.lfo[0], lfoTarget);
            break;

        case 5:
            if (osc_.lfo[0].rate > kMinLfoRate)
                osc_lfo_setRate(osc_, osc_.lfo[0], osc_.lfo[0].rate - 1);
            break;

        case 6:
            if (osc_.lfo[0].depth > kMinLfoDepth)
                osc_lfo_setDepth(osc_, osc_.lfo[0], osc_.lfo[0].depth - 1);
            break;

        case 7:
            osc_lfo_setLoop(osc_, osc_.lfo[0], !osc_.lfo[0].loop);
            break;
        }
        break;

    case OSC_A3_MENU:
    case OSC_B3_MENU:
        switch (menuTab) {
        case 0:
            osc_lfo_setActive(osc_, osc_.lfo[1], !osc_.lfo[1].active);
            break;

        case 2:
            (osc_.lfo[1].type > kMinLfoType) ? osc_lfo_setType(osc_, osc_.lfo[1], osc_.lfo[1].type - 1) : osc_lfo_setType(osc_, osc_.lfo[1], kMaxLfoType);
            break;

        case 4:
            switch (osc_.lfo[1].target) {
            case 0:
                switch (osc_.lfo[0].target) {
                case 1:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 1;
                    break;
                }
                break;

            case 1:
                switch (osc_.lfo[0].target) {
                case 0:
                    lfoTarget = 2;
                    break;

                case 2:
                    lfoTarget = 0;
                    break;
                }
                break;

            case 2:
                switch (osc_.lfo[0].target) {
                case 0:
                    lfoTarget = 1;
                    break;

                case 1:
                    lfoTarget = 0;
                    break;
                }
                break;
            }

            osc_lfo_setTarget(osc_, osc_.lfo[1], lfoTarget);
            break;

        case 5:
            if (osc_.lfo[1].rate > kMinLfoRate)
                osc_lfo_setRate(osc_, osc_.lfo[1], osc_.lfo[1].rate - 1);
            break;

        case 6:
            if (osc_.lfo[1].depth > kMinLfoDepth)
                osc_lfo_setDepth(osc_, osc_.lfo[1], osc_.lfo[1].depth - 1);
            break;

        case 7:
            osc_lfo_setLoop(osc_, osc_.lfo[1], !osc_.lfo[1].loop);
            break;
        }
        break;

    default:
        break;
    }
}

void Controller::osc_setActive(Osc &osc_, bool active_) {
    // update data
    osc_.active = active_;
    // update lcd
    if ((((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) && (osc_.number == 0)) || (((menu == OSC_B0_MENU) || (menu == OSC_A1_MENU)) && (osc_.number == 1)))
        lcd_drawOsc_ActiveData(osc_);
    lcd_drawInfo_Osc_ActiveData(osc_);
    lcd_drawInfo_Osc_GraphData(osc_);
}

void Controller::osc_setWavetableSelected(Osc &osc_, int16_t wavetable_) {
    if ((wavetable_ >= -1) && (wavetable_ < wavetableLibrarySize)) {
        // disable keyboard
        keyboard_disable();
        // update data
        osc_.wavetableSelected = wavetable_;
        // clear data
        memset(&(osc_.wavetableSelectedData), 0x00, sizeof(WavetableData));
        memset(&(osc_.wavSelectedData), 0x00, sizeof(WavData));
        if (wavetable_ != -1) {
            // set num data
            sprintf(osc_.wavetableSelectedData.num, "%04d",
                    osc_.wavetableSelected + 1);
            // read sd data
            char temp[kFileNameSize + 1];
            f_close(&sd.file);
            if ((f_open(&sd.file, "System/Data/Wavetable.lib", FA_READ) == FR_OK) && (f_lseek(&sd.file, 25 + (osc_.wavetableSelected * 32)) == FR_OK) &&
                (f_read(&sd.file, temp, kFileNameSize, &sd.bytesread) == FR_OK)) {
                f_close(&sd.file);
                // write fileName
                strncpy(osc_.wavetableSelectedData.fileName, temp, kFileNameSize);
                // write nameLongR
                strncpy(osc_.wavetableSelectedData.nameLongR, temp, kFileNameSize);
                // write nameShort
                char *temp2 = strtok(temp, ".");
                uint8_t length = strlen(temp2);
                if (length > 10) {
                    strncpy(osc_.wavetableSelectedData.nameShortR, temp2, 9);
                    strcat(osc_.wavetableSelectedData.nameShortR, "_");
                } else if (length == 10) {
                    strncpy(osc_.wavetableSelectedData.nameShortR, temp2, length);
                } else {
                    char bText[] = "          ";
                    uint8_t bLength = 10 - length;
                    strncpy(osc_.wavetableSelectedData.nameShortR, bText, bLength);
                    strncat(osc_.wavetableSelectedData.nameShortR, temp2, length);
                }
                // update sdram
                bool wavetableReadError = false;
                bool wavetableTypeError = false;
                // read sd file
                char fileName[] = "Wavetable/";
                strcat(fileName, osc_.wavetableSelectedData.fileName);
                if (f_open(&sd.file, fileName, FA_READ) == FR_OK) {
                    if (f_read(&sd.file, &osc_.wavSelectedData.riff_chunk, 12, &sd.bytesread) == FR_OK) {
                        // read riff_chunk
                        if ((osc_.wavSelectedData.riff_chunk.chunkId == 0x46464952) && (osc_.wavSelectedData.riff_chunk.fileFormat == 0x45564157)) {
                            uint32_t chunkSize =
                                osc_.wavSelectedData.riff_chunk.chunkSize + 8;
                            // read fmt_chunk
                            for (uint32_t i = 12; i < (chunkSize - 24); i++) {
                                f_lseek(&sd.file, i);
                                f_read(&sd.file, &osc_.wavSelectedData.fmt_chunk, 24, &sd.bytesread);
                                if (osc_.wavSelectedData.fmt_chunk.chunkId == 0x20746D66) {
                                    osc_.wavSelectedData.fmt_chunk.chunkStartByte = i;
                                    // check(osc.wavSelectedData.fmt_chunk.chunkStartByte, 0);
                                    break;
                                }
                            }
                            // check fmt_chunk
                            if ((osc_.wavSelectedData.fmt_chunk.chunkId == 0x20746D66) && ((osc_.wavSelectedData.fmt_chunk.chunkSize == 16) || (osc_.wavSelectedData.fmt_chunk.chunkSize == 18) || (osc_.wavSelectedData.fmt_chunk.chunkSize == 40)) &&
                                ((osc_.wavSelectedData.fmt_chunk.audioFormat == 0x01) || (osc_.wavSelectedData.fmt_chunk.audioFormat == 0x03)) &&
                                ((osc_.wavSelectedData.fmt_chunk.nbrChannels == 0x01) || (osc_.wavSelectedData.fmt_chunk.nbrChannels == 0x02)) &&
                                ((osc_.wavSelectedData.fmt_chunk.bitPerSample == 8) || (osc_.wavSelectedData.fmt_chunk.bitPerSample == 16) ||
                                 (osc_.wavSelectedData.fmt_chunk.bitPerSample == 24) || (osc_.wavSelectedData.fmt_chunk.bitPerSample == 32))) {
                                // read data_chunk
                                for (uint32_t j = 0; j < (chunkSize - 8); j++) {
                                    f_lseek(&sd.file, j);
                                    f_read(&sd.file, &osc_.wavSelectedData.data_chunk, 8, &sd.bytesread);
                                    if (osc_.wavSelectedData.data_chunk.chunkId == 0x61746164) {
                                        osc_.wavSelectedData.data_chunk.chunkStartByte = j;
                                        // check(osc.wavSelectedData.data_chunk.chunkStartByte, 0);
                                        break;
                                    }
                                }
                                // check data_chunk
                                if (osc_.wavSelectedData.data_chunk.chunkId == 0x61746164) {
                                    // analyze data
                                    osc_.wavetableSelectedData.coefSampleSize = 1.0f;
                                    // calculate sampleSize
                                    switch (osc_.wavSelectedData.fmt_chunk.nbrChannels) {
                                    case 0x01:
                                        osc_.wavetableSelectedData.coefSampleSize *= 1;
                                        osc_.wavetableSelectedData.channel = CH_MONO;
                                        break;

                                    case 0x02:
                                        osc_.wavetableSelectedData.coefSampleSize *= 0.5;
                                        osc_.wavetableSelectedData.channel = CH_STEREO;
                                        break;
                                    }

                                    switch (osc_.wavSelectedData.fmt_chunk.bitPerSample) {
                                    case 8:
                                        osc_.wavetableSelectedData.coefSampleSize *= 1;
                                        osc_.wavetableSelectedData.bitdepth = BD_08;
                                        break;

                                    case 16:
                                        osc_.wavetableSelectedData.coefSampleSize /= 2;
                                        osc_.wavetableSelectedData.bitdepth = BD_16;
                                        break;

                                    case 24:
                                        osc_.wavetableSelectedData.coefSampleSize /= 3;
                                        osc_.wavetableSelectedData.bitdepth = BD_24;
                                        break;

                                    case 32:
                                        osc_.wavetableSelectedData.coefSampleSize /= 4;
                                        osc_.wavetableSelectedData.bitdepth = BD_32;
                                        break;
                                    }

                                    osc_.wavetableSelectedData.sampleByteSize = osc_.wavSelectedData.data_chunk.chunkSize;
                                    osc_.wavetableSelectedData.sampleSize = osc_.wavetableSelectedData.sampleByteSize * osc_.wavetableSelectedData.coefSampleSize;
                                    osc_.wavetableSelectedData.stepSize = osc_.wavetableSelectedData.sampleSize / kWavetableSampleSize;
                                    uint32_t remainder = osc_.wavetableSelectedData.sampleSize % kWavetableSampleSize;

                                    if (remainder == 0) {
                                        switch (osc_.wavetableSelectedData.stepSize) {
                                        case 1:
                                            osc_.wavetableSelectedData.step = ST_001;
                                            break;

                                        case 2:
                                            osc_.wavetableSelectedData.step = ST_002;
                                            break;

                                        case 4:
                                            osc_.wavetableSelectedData.step = ST_004;
                                            break;

                                        case 8:
                                            osc_.wavetableSelectedData.step = ST_008;
                                            break;

                                        case 16:
                                            osc_.wavetableSelectedData.step = ST_016;
                                            break;

                                        case 24:
                                            osc_.wavetableSelectedData.step = ST_024;
                                            break;

                                        case 32:
                                            osc_.wavetableSelectedData.step = ST_032;
                                            break;

                                        case 40:
                                            osc_.wavetableSelectedData.step = ST_040;
                                            break;

                                        case 48:
                                            osc_.wavetableSelectedData.step = ST_048;
                                            break;

                                        case 56:
                                            osc_.wavetableSelectedData.step = ST_056;
                                            break;

                                        case 64:
                                            osc_.wavetableSelectedData.step = ST_064;
                                            break;

                                        case 72:
                                            osc_.wavetableSelectedData.step = ST_072;
                                            break;

                                        case 80:
                                            osc_.wavetableSelectedData.step = ST_080;
                                            break;

                                        case 88:
                                            osc_.wavetableSelectedData.step = ST_088;
                                            break;

                                        case 96:
                                            osc_.wavetableSelectedData.step = ST_096;
                                            break;

                                        case 104:
                                            osc_.wavetableSelectedData.step = ST_104;
                                            break;

                                        case 112:
                                            osc_.wavetableSelectedData.step = ST_112;
                                            break;

                                        case 120:
                                            osc_.wavetableSelectedData.step = ST_120;
                                            break;

                                        case 128:
                                            osc_.wavetableSelectedData.step = ST_128;
                                            break;

                                        case 136:
                                            osc_.wavetableSelectedData.step = ST_136;
                                            break;

                                        case 144:
                                            osc_.wavetableSelectedData.step = ST_144;
                                            break;

                                        case 152:
                                            osc_.wavetableSelectedData.step = ST_152;
                                            break;

                                        case 160:
                                            osc_.wavetableSelectedData.step = ST_160;
                                            break;

                                        case 168:
                                            osc_.wavetableSelectedData.step = ST_168;
                                            break;

                                        case 176:
                                            osc_.wavetableSelectedData.step = ST_176;
                                            break;

                                        case 184:
                                            osc_.wavetableSelectedData.step = ST_184;
                                            break;

                                        case 192:
                                            osc_.wavetableSelectedData.step = ST_192;
                                            break;

                                        case 200:
                                            osc_.wavetableSelectedData.step = ST_200;
                                            break;

                                        case 208:
                                            osc_.wavetableSelectedData.step = ST_208;
                                            break;

                                        case 216:
                                            osc_.wavetableSelectedData.step = ST_216;
                                            break;

                                        case 224:
                                            osc_.wavetableSelectedData.step = ST_224;
                                            break;

                                        case 232:
                                            osc_.wavetableSelectedData.step = ST_232;
                                            break;

                                        case 240:
                                            osc_.wavetableSelectedData.step = ST_240;
                                            break;

                                        case 248:
                                            osc_.wavetableSelectedData.step = ST_248;
                                            break;

                                        case 256:
                                            osc_.wavetableSelectedData.step = ST_256;
                                            break;

                                        default:
                                            osc_.wavetableSelectedData.step = ST_NA;
                                            break;
                                        }
                                    } else {
                                        osc_.wavetableSelectedData.step = ST_NA;
                                    }

                                    if (osc_.wavetableSelectedData.step != ST_NA) {
                                    } else {
                                        wavetableTypeError = true;
                                    }
                                }
                            } else {
                                wavetableTypeError = true;
                            }
                        } else {
                            wavetableReadError = true;
                        }
                    } else {
                        wavetableReadError = true;
                    }
                } else {
                    wavetableReadError = true;
                }

                osc_.wavetableSelectedReadError = wavetableReadError;
                osc_.wavetableSelectedTypeError = wavetableTypeError;
            }
        } else {
            // clear data
            memset(osc_.wavetableSelectedData.num, '-', 4);
            memcpy(osc_.wavetableSelectedData.nameShortR, kDataDashR, 11);

            osc_.wavetableSelectedData.channel = CH_NA;
            osc_.wavetableSelectedData.bitdepth = BD_NA;
            osc_.wavetableSelectedData.step = ST_NA;

            osc_.wavetableSelectedData.coefSampleSize = 0;
            osc_.wavetableSelectedData.sampleByteSize = 0;
            osc_.wavetableSelectedData.sampleSize = 0;
            osc_.wavetableSelectedData.stepSize = 0;

            osc_.wavetableSelectedReadError = false;
            osc_.wavetableSelectedTypeError = false;
        }
        f_close(&sd.file);
        // enable keyboard
        keyboard_enable();
        // update lcd
        if ((((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) && (osc_.number == 0)) || (((menu == OSC_B0_MENU) || (menu == OSC_B1_MENU)) && (osc_.number == 1)))
            lcd_drawOsc_WavetableData(osc_);
    }
}

void Controller::osc_setWavetableLoaded(Osc &osc_, bool view_) {
    if ((!osc_.wavetableSelectedReadError) && (!osc_.wavetableSelectedTypeError) && (osc_.wavetableSelected != osc_.wavetableLoaded)) {
        // disable keyboard
        keyboard_disable();
        if (osc_.wavetableSelected != -1) {
            // clear data
            memset(&(osc_.wavetableLoadedData), 0x00, sizeof(WavetableData));
            memset(&(osc_.wavLoadedData), 0x00, sizeof(WavData));
            // copy data
            osc_.wavetableLoaded = osc_.wavetableSelected;
            osc_.wavetableLoadedData = osc_.wavetableSelectedData;
            osc_.wavLoadedData = osc_.wavSelectedData;

            if (osc_.wavetableLoadedData.step != ST_NA) {
                if ((((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) && (osc_.number == 0)) || (((menu == OSC_B0_MENU) || (menu == OSC_B1_MENU)) && (osc_.number == 1))) {
                    lcd_setMenuNumState(kLayerColorPalette[9]);
                    lcd.drawText("    LOADING", 11, kMenuData4X[1], kMenuHeaderY + 23);
                }
                // read sample
                uint32_t rawDataSize = osc_.wavetableLoadedData.sampleByteSize;
                uint32_t rawArraySize = 96000;
                uint32_t rawArrayCounterMax = rawDataSize / rawArraySize;
                uint32_t rawArrayRemainder = rawDataSize % rawArraySize;

                uint8_t rawArray[rawArraySize];

                uint8_t coefChannel;
                uint8_t coefBps;

                uint32_t sdramDataCounter = 0;

                switch (osc_.wavLoadedData.fmt_chunk.nbrChannels) {
                case 1:
                    coefChannel = 1;
                    break;
                case 2:
                    coefChannel = 2;
                    break;
                }

                switch (osc_.wavLoadedData.fmt_chunk.bitPerSample) {
                case 8:
                    coefBps = 1;
                    break;

                case 16:
                    coefBps = 2;
                    break;

                case 24:
                    coefBps = 3;
                    break;

                case 32:
                    coefBps = 4;
                    break;
                }

                uint32_t offsetSize = coefChannel * coefBps;
                uint32_t readArraySize;
                uint32_t readSampleSize;
                uint32_t tempSampleSize;

                int32_t minData = 0;
                int32_t maxData = 0;
                int32_t limitData;

                // read sd file

                char fileName[100] = "Wavetable/";
                strcat(fileName, osc_.wavetableLoadedData.fileName);

                f_close(&sd.file);
                if ((f_open(&sd.file, fileName, FA_READ) == FR_OK) && (f_lseek(&sd.file, osc_.wavLoadedData.data_chunk.chunkStartByte + 8) == FR_OK)) {
                    (osc_.playWavetableSector == 0) ? osc_.writeWavetableSector = 1 : osc_.writeWavetableSector = 0;
                    osc_.wavetableSector[osc_.writeWavetableSector].size = osc_.wavetableLoadedData.sampleSize - 1;
                    for (uint32_t i = 0; i <= rawArrayCounterMax; i++) {
                        // file to raw
                        (i < rawArrayCounterMax) ? readArraySize = rawArraySize : readArraySize = rawArrayRemainder;
                        if (f_read(&sd.file, (char *)&rawArray, readArraySize, &sd.bytesread) == FR_OK) {

                            // raw to temp
                            readSampleSize = readArraySize / offsetSize;
                            int16_t writeData;

                            for (int j = 0; j < readSampleSize; j++) {
                                if (osc_.wavLoadedData.fmt_chunk.bitPerSample == 8) {
                                    int8_t *dataPtr = (int8_t *)&rawArray[j * offsetSize];
                                    int8_t temp = *dataPtr - 128;
                                    int16_t data = (int16_t)(temp << 8);
                                    if (temp >> 7)
                                        data |= (0xFF << 16);
                                    writeData = data;
                                } else if (osc_.wavLoadedData.fmt_chunk.bitPerSample == 16) {
                                    int16_t *dataPtr = (int16_t *)&rawArray[j * offsetSize];
                                    writeData = (int16_t)*dataPtr;
                                } else if (osc_.wavLoadedData.fmt_chunk.bitPerSample == 24) {
                                    int16_t *dataPtr = (int16_t *)&rawArray[(j * offsetSize) + 1];
                                    writeData = (int16_t)*dataPtr;
                                } else if (osc_.wavLoadedData.fmt_chunk.bitPerSample == 32) {
                                    float *dataPtr = (float *)&rawArray[j * offsetSize];
                                    int16_t data = (int16_t)((*dataPtr) * 32767);
                                    writeData = data;
                                }

                                // temp to sdram
                                if (writeData < minData)
                                    minData = writeData;
                                if (writeData > maxData)
                                    maxData = writeData;
                                sdram_write16BitAudio(kRamWavetableAddressLibrary[osc_.number][osc_.writeWavetableSector] + (sdramDataCounter * 2), writeData);
                                sdramDataCounter += 1;
                            }
                        }
                    }

                    minData *= -1;
                    (minData > maxData) ? limitData = minData : limitData = maxData;
                    osc_.wavetableSector[osc_.writeWavetableSector].normMultiplier = 32767.0f / limitData;

                    osc_.playWavetableSector = osc_.writeWavetableSector;
                    osc_.writeWavetableSector != osc_.writeWavetableSector;

                    osc_.start = kStepDataLibrary[osc_.wavetableLoadedData.step].min;
                    osc_.end = kStepDataLibrary[osc_.wavetableLoadedData.step].max;
                    osc_.waveStart = kStepDataLibrary[osc_.wavetableLoadedData.step].data[osc_.start];
                    osc_.waveEnd = kStepDataLibrary[osc_.wavetableLoadedData.step].data[osc_.end];

                    osc_.wavetablePlay = true;

                    /*
                    lcd.setForeColor(WHITE);
                    lcd.setFont(FONT_05x07);
                    lcd.setAlignment(LEFT);

                    lcd.drawNumber(osc.playWavetableSector, 2, 20, 50);
                    lcd.drawNumber(osc.wavetableSector[osc.playWavetableSector].size, 6,
                    20, 65);
                    // lcd.drawNumber(readArraySize, 6, 20, 65);
                    // lcd.drawNumber(readSampleSize, 6, 20, 80);
                    // lcd.drawNumber(offsetSize, 6, 20, 95);
                    // lcd.drawNumber(rawArrayCounterMax, 6, 20, 110);
                    // lcd.drawNumber(rawArrayRemainder, 6, 20, 125);

                    for (uint8_t i = 0; i < 20; i++) {
                        uint32_t address =
                    (uint32_t)(kRamWavetableAddressLibrary[osc.number][osc.writeWavetableSector]
                    + (i * 2)); int16_t x = sdram_read16BitAudio(address);
                        lcd.drawNumber(x, 10, 20, 80 + (i * 15));
                    }
                    */
                }
            }
        } else {
            osc_.wavetableLoaded = osc_.wavetableSelected;
            osc_.wavetableLoadedData = osc_.wavetableSelectedData;
            osc_.wavLoadedData = osc_.wavSelectedData;
            osc_.start = 0;
            osc_.end = 0;
            osc_.waveStart = 0;
            osc_.waveEnd = 0;
            osc_.wavetablePlay = false;
        }
    }

    f_close(&sd.file);
    // enable keyboard
    keyboard_enable();
    // update lcd
    if (view_) {
        if ((((menu == OSC_A0_MENU) || (menu == OSC_A1_MENU)) && (osc_.number == 0)) || (((menu == OSC_B0_MENU) || (menu == OSC_B1_MENU)) && (osc_.number == 1)))
            lcd_drawOsc_WavetableData(osc_);
        if (((menu == OSC_A1_MENU) && (osc_.number == 0)) || ((menu == OSC_B1_MENU) && (osc_.number == 1))) {
            lcd_drawOsc_StartData(osc_);
            lcd_drawOsc_EndData(osc_);
        }
        lcd_drawInfo_Osc_WavetableData(osc_);
        lcd_drawInfo_Osc_GraphData(osc_);
        if (playActive) {
            lcd.setForeColor(playColor);
            lcd.drawHLine(kPlayX, kPlayY, playX);
            (playColor == kPlayColor0) ? lcd.setForeColor(kPlayColor1) : lcd.setForeColor(kPlayColor0);
            lcd.drawHLine(kPlayX + playX + 1, kPlayY, kPlayWidth - playX);
        }
    }
}

void Controller::osc_setLevel(Osc &osc_, uint8_t level_) {
    if ((level_ >= kMinOscLevel) && (level_ <= kMaxOscLevel)) {
        // update data
        osc_.level = level_;
        // update lcd
        if (((menu == OSC_A0_MENU) && (osc_.number == 0)) || ((menu == OSC_B0_MENU) && (osc_.number == 1)))
            lcd_drawOsc_LevelData(osc_);
        lcd_drawInfo_Osc_LevelData(osc_);
    }
}

void Controller::osc_setTune(Osc &osc_, uint8_t tune_) {
    if ((tune_ >= kMinOscTune) && (tune_ <= kMaxOscTune)) {
        // update data
        osc_.tune = tune_;
        // update lcd
        if (((menu == OSC_A0_MENU) && (osc_.number == 0)) || ((menu == OSC_B0_MENU) && (osc_.number == 1)))
            lcd_drawOsc_TuneData(osc_);
        lcd_drawInfo_Osc_TuneData(osc_);
    }
}

void Controller::osc_setPhase(Osc &osc_, uint8_t phase_) {
    if ((phase_ >= kMinOscPhase) && (phase_ <= kMaxOscPhase)) {
        // update data
        osc_.phase = phase_;
        // update lcd
        if (((menu == OSC_A0_MENU) && (osc_.number == 0)) || ((menu == OSC_B0_MENU) && (osc_.number == 1)))
            lcd_drawOsc_PhaseData(osc_);
        lcd_drawInfo_Osc_PhaseData(osc_);
    }
}

void Controller::osc_setNormalize(Osc &osc_, bool norm_) {
    // update data
    osc_.normalize = norm_;
    // update lcd
    if (((menu == OSC_A0_MENU) && (osc_.number == 0)) || ((menu == OSC_B0_MENU) && (osc_.number == 1)))
        lcd_drawOsc_NormalizeData(osc_);
}

void Controller::osc_setStart(Osc &osc_, uint16_t start_) {
    if ((start_ >= kStepDataLibrary[osc_.wavetableLoadedData.step].min) && (start_ <= kStepDataLibrary[osc_.wavetableLoadedData.step].max)) {
        // update data
        osc_.start = start_;
        osc_.waveStart = kStepDataLibrary[osc_.wavetableLoadedData.step].data[osc_.start];
        // update lcd
        if (((menu == OSC_A1_MENU) && (osc_.number == 0)) || ((menu == OSC_B1_MENU) && (osc_.number == 1)))
            lcd_drawOsc_StartData(osc_);
        lcd_drawInfo_Osc_GraphData(osc_);
    }
}

void Controller::osc_setEnd(Osc &osc_, uint16_t end_) {
    if ((end_ >= kStepDataLibrary[osc_.wavetableLoadedData.step].min) && (end_ <= kStepDataLibrary[osc_.wavetableLoadedData.step].max)) {
        // update data
        osc_.end = end_;
        osc_.waveEnd = kStepDataLibrary[osc_.wavetableLoadedData.step].data[osc_.end];
        // update lcd
        if (((menu == OSC_A1_MENU) && (osc_.number == 0)) || ((menu == OSC_B1_MENU) && (osc_.number == 1)))
            lcd_drawOsc_EndData(osc_);
        lcd_drawInfo_Osc_GraphData(osc_);
    }
}

void Controller::osc_setXFlip(Osc &osc_, bool xFlip_) {
    // update data
    osc_.xFlip = xFlip_;
    // update lcd
    if (((menu == OSC_A1_MENU) && (osc_.number == 0)) || ((menu == OSC_B1_MENU) && (osc_.number == 1)))
        lcd_drawOsc_XFlipData(osc_);
    lcd_drawInfo_Osc_GraphData(osc_);
}

void Controller::osc_setYFlip(Osc &osc_, bool yFlip_) {
    // update data
    osc_.yFlip = yFlip_;
    // update lcd
    if (((menu == OSC_A1_MENU) && (osc_.number == 0)) || ((menu == OSC_B1_MENU) && (osc_.number == 1)))
        lcd_drawOsc_YFlipData(osc_);
    lcd_drawInfo_Osc_GraphData(osc_);
}

void Controller::osc_lfo_setActive(Osc &osc_, Lfo &lfo_, uint8_t active_) {
    // update data
    lfo_.active = active_;
    // update lcd
    if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
        ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
        lcd_drawOscLfo_ActiveData(osc_, lfo_);
    lcd_drawInfo_OscLfo_ActiveData(osc_, lfo_);
    lcd_drawInfo_OscLfo_TypeData(osc_, lfo_);
    lcd_drawInfo_OscLfo_GraphData(osc_, lfo_);
}

void Controller::osc_lfo_setType(Osc &osc_, Lfo &lfo_, uint8_t type_) {
    if ((type_ >= kMinLfoType) && (type_ <= kMaxLfoType)) {
        // update data
        lfo_.type = type_;
        // update lcd
        if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
            ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
            lcd_drawOscLfo_TypeData(osc_, lfo_);
        lcd_drawInfo_OscLfo_TypeData(osc_, lfo_);
        lcd_drawInfo_OscLfo_GraphData(osc_, lfo_);
    }
}

void Controller::osc_lfo_setTarget(Osc &osc_, Lfo &lfo_, uint8_t target_) {
    if ((target_ >= kMinLfoTarget) && (target_ <= kMaxLfoTarget)) {
        // update data
        lfo_.target = target_;
        // update lcd
        if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
            ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
            lcd_drawOscLfo_TargetData(osc_, lfo_);
        lcd_drawInfo_OscLfo_TargetData(osc_, lfo_);
    }
}

void Controller::osc_lfo_setRate(Osc &osc_, Lfo &lfo_, uint8_t rate_) {
    if ((rate_ >= kMinLfoRate) && (rate_ <= kMaxLfoRate)) {
        // update data
        lfo_.rate = rate_;
        // update lcd
        if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
            ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
            lcd_drawOscLfo_RateData(osc_, lfo_);
        lcd_drawInfo_OscLfo_RateData(osc_, lfo_);
    }
}

void Controller::osc_lfo_setDepth(Osc &osc_, Lfo &lfo_, uint8_t depth_) {
    if ((depth_ >= kMinLfoDepth) && (depth_ <= kMaxLfoDepth)) {
        // update data
        lfo_.depth = depth_;
        // update lcd
        if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
            ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
            lcd_drawOscLfo_DepthData(osc_, lfo_);
    }
}

void Controller::osc_lfo_setLoop(Osc &osc_, Lfo &lfo_, bool loop_) {
    // update data
    lfo_.loop = loop_;
    // update lcd
    if (((menu == OSC_A2_MENU) && (osc_.number == 0) && (lfo_.number == 0)) || ((menu == OSC_A3_MENU) && (osc_.number == 0) && (lfo_.number == 1)) ||
        ((menu == OSC_B2_MENU) && (osc_.number == 1) && (lfo_.number == 0)) || ((menu == OSC_B3_MENU) && (osc_.number == 1) && (lfo_.number == 1)))
        lcd_drawOscLfo_LoopData(osc_, lfo_);
}

/* Filter functions ----------------------------------------------------------*/

void Controller::filter_select() {
    if (menu == FILTER_0_MENU) {
        preMenu = menu;
        menu = FILTER_1_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFilterMenu(1);
    } else if (menu == FILTER_1_MENU) {
        preMenu = menu;
        menu = FILTER_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFilterMenu(0);
    } else {
        preMenu = menu;
        menu = FILTER_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawFilterMenu(0);
    }
}

void Controller::filter_reset(uint8_t filterNum_) {
    filter_setActive(filterNum_, kInitialFilterActive);
    filter_setType(filterNum_, kInitialFilterType);
    filter_setFreq(filterNum_, kInitialFilterFreq);
    filter_setRes(filterNum_, kInitialFilterRes);
    filter_setSlope(filterNum_, kInitialFilterSlope);
    filter_setDry(filterNum_, kInitialFilterDry);
    filter_setWet(filterNum_, kInitialFilterWet);

    Filter &filter_ = filter[filterNum_];

    filter_.dataIn[0] = 0;
    filter_.dataIn[1] = 0;
    filter_.dataIn[2] = 0;

    filter_.dataOut[0] = 0;
    filter_.dataOut[1] = 0;
    filter_.dataOut[2] = 0;
}

void Controller::filter_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::filter_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::filter_menuUp() {
    uint8_t filterNum;
    (menu == FILTER_0_MENU) ? filterNum = 0 : filterNum = 1;

    switch (menuTab) {
    case 0:
        filter_setActive(filterNum, !filter[filterNum].active);
        break;

    case 2:
        if (filter[filterNum].type < kMaxFilterType) {
            filter_setType(filterNum, filter[filterNum].type + 1);
        } else {
            filter_setType(filterNum, kMinFilterType);
        }
        break;

    case 3:
        if (filter[filterNum].freq < kMaxFilterFreq) {
            filter_setFreq(filterNum, filter[filterNum].freq + 1);
        }
        break;

    case 4:
        if (filter[filterNum].res < kMaxFilterRes) {
            filter_setRes(filterNum, filter[filterNum].res + 1);
        }
        break;

    case 5:
        if (filter[filterNum].slope < kMaxFilterSlope) {
            filter_setSlope(filterNum, filter[filterNum].slope + 1);
        }
        break;

    case 6:
        if (filter[filterNum].dry < kMaxFilterDry) {
            filter_setDry(filterNum, filter[filterNum].dry + 1);
        } else if ((filter[filterNum].dry == kMaxFilterDry) && (filter[filterNum].wet > kMinFilterWet)) {
            filter_setWet(filterNum, filter[filterNum].wet - 1);
        }
        break;

    case 7:
        if (filter[filterNum].wet < kMaxFilterWet) {
            filter_setWet(filterNum, filter[filterNum].wet + 1);
        } else if ((filter[filterNum].wet == kMaxFilterWet) && (filter[filterNum].dry > kMinFilterDry)) {
            filter_setDry(filterNum, filter[filterNum].dry - 1);
        }
        break;
    }
}

void Controller::filter_menuDown() {
    uint8_t filterNum;
    (menu == FILTER_0_MENU) ? filterNum = 0 : filterNum = 1;

    switch (menuTab) {
    case 0:
        filter_setActive(filterNum, !filter[filterNum].active);
        break;

    case 2:
        if (filter[filterNum].type > kMinFilterType) {
            filter_setType(filterNum, filter[filterNum].type - 1);
        } else {
            filter_setType(filterNum, kMaxFilterType);
        }
        break;

    case 3:
        if (filter[filterNum].freq > kMinFilterFreq) {
            filter_setFreq(filterNum, filter[filterNum].freq - 1);
        }
        break;

    case 4:
        if (filter[filterNum].res > kMinFilterRes) {
            filter_setRes(filterNum, filter[filterNum].res - 1);
        }
        break;

    case 5:
        if (filter[filterNum].slope > kMinFilterSlope) {
            filter_setSlope(filterNum, filter[filterNum].slope - 1);
        }
        break;

    case 6:
        if (filter[filterNum].dry > kMinFilterDry) {
            filter_setDry(filterNum, filter[filterNum].dry - 1);
        }
        break;

    case 7:
        if (filter[filterNum].wet > kMinFilterWet) {
            filter_setWet(filterNum, filter[filterNum].wet - 1);
        }
        break;
    }
}

void Controller::filter_setActive(uint8_t filterNum_, bool active_) {
    Filter &filter_ = filter[filterNum_];

    if ((((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) && (!filter_.genTransition.active)) {
        // update data
        // if (active_) filter_.cleanMemory();
        filter_genTransition(filterNum_, FIL_MODE_ACTIVE, filter_.active, active_, filter_.type, filter_.type);
        filter_.active = active_;
        // update lcd
        lcd_drawFilter_ActiveData(filterNum_);
        lcd_drawInfo_Filter_ActiveData(filterNum_);
        lcd_drawInfo_Filter_TypeData(filterNum_);
    } else if (!filter[filterNum_].genTransition.active) {
        // update data
        filter_.active = active_;
        // update lcd
        lcd_drawInfo_Filter_ActiveData(filterNum_);
        lcd_drawInfo_Filter_TypeData(filterNum_);
        // update lcd
        if (menu == MAIN_MENU) {
            switch (filterNum_) {
            case 0:
                lcd_drawMain_Filter0Data();
                break;

            case 1:
                lcd_drawMain_Filter1Data();
                break;
            }
        }
    }
}

void Controller::filter_setType(uint8_t filterNum_, uint8_t type_) {
    Filter &filter_ = filter[filterNum_];

    if ((((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) && (!filter_.genTransition.active)) {
        // update data
        if (filter_.active) {
            filter_genTransition(filterNum_, FIL_MODE_TYPE, filter_.active, filter_.active, filter_.type, type_);
            filter_.type = type_;
        } else {
            filter_.type = type_;
            filter_.calculateCoef();
        }
        // update lcd
        lcd_drawFilter_TypeData(filterNum_);
        lcd_drawInfo_Filter_TypeData(filterNum_);
    } else if (!filter[filterNum_].genTransition.active) {
        // update data
        filter_.type = type_;
        filter_.calculateCoef();
        // update lcd
        lcd_drawInfo_Filter_TypeData(filterNum_);
        // update lcd
        if (menu == MAIN_MENU) {
            switch (filterNum_) {
            case 0:
                lcd_drawMain_Filter0Data();
                break;

            case 1:
                lcd_drawMain_Filter1Data();
                break;
            }
        }
    }
}

void Controller::filter_setFreq(uint8_t filterNum_, uint8_t freq_) {
    if ((freq_ >= kMinFilterFreq) && (freq_ <= kMaxFilterFreq)) {
        // update data
        filter[filterNum_].freq = freq_;
        filter[filterNum_].calculateCoef();

        // update lcd
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_FreqData(filterNum_);
        lcd_drawInfo_Filter_FreqData(filterNum_);
    }
}

void Controller::filter_setRes(uint8_t filterNum_, uint8_t res_) {
    if ((res_ >= kMinFilterRes) && (res_ <= kMaxFilterRes)) {
        // update data
        filter[filterNum_].res = res_;
        filter[filterNum_].calculateCoef();

        // update lcd
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_ResData(filterNum_);
        lcd_drawInfo_Filter_ResData(filterNum_);
    }
}

void Controller::filter_setSlope(uint8_t filterNum_, uint8_t slope_) {
    if ((slope_ >= kMinFilterSlope) && (slope_ <= kMaxFilterSlope)) {
        // update data
        filter[filterNum_].slope = slope_;
        filter[filterNum_].calculateCoef();

        // update lcd
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1)))
            lcd_drawFilter_SlopeData(filterNum_);
    }
}

void Controller::filter_setDry(uint8_t filterNum_, uint8_t dry_) {
    Filter &filter_ = filter[filterNum_];

    if ((dry_ >= kMinFilterDry) && (dry_ <= kMaxFilterDry) && (!filter_.mixTransition.active)) {
        // update data
        uint8_t targetDry = dry_;
        uint8_t targetWet = filter_.wet;

        if ((filter_.limitMix) &&
            ((targetDry + targetWet) > filter_.limitMixData)) {
            targetWet = filter_.limitMixData - targetDry;
        }

        filter_.dry = targetDry;
        filter_.wet = targetWet;

        float targetDryFloat = kFilterMixDataLibrary[filter_.dry].data;
        float targetWetFloat = kFilterMixDataLibrary[filter_.wet].data;

        if (filter_.active) {
            filter_mixTransition(filterNum_, targetDryFloat, targetWetFloat);
        } else {
            filter_.dryFloat = kFilterMixDataLibrary[filter_.dry].data;
            filter_.wetFloat = kFilterMixDataLibrary[filter_.wet].data;
        }

        // update lcd
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) {
            lcd_drawFilter_DryData(filterNum_);
            lcd_drawFilter_WetData(filterNum_);
        }
    }
}

void Controller::filter_setWet(uint8_t filterNum_, uint8_t wet_) {
    Filter &filter_ = filter[filterNum_];

    if ((wet_ >= kMinFilterWet) && (wet_ <= kMaxFilterWet) && (!filter_.mixTransition.active)) {
        // update data
        uint8_t targetDry = filter_.dry;
        uint8_t targetWet = wet_;

        if ((filter_.limitMix) &&
            ((targetDry + targetWet) > filter_.limitMixData)) {
            targetDry = filter_.limitMixData - targetWet;
        }

        filter_.dry = targetDry;
        filter_.wet = targetWet;

        float targetDryFloat = kFilterMixDataLibrary[filter_.dry].data;
        float targetWetFloat = kFilterMixDataLibrary[filter_.wet].data;

        if (filter_.active) {
            filter_mixTransition(filterNum_, targetDryFloat, targetWetFloat);
        } else {
            filter_.dryFloat = kFilterMixDataLibrary[filter_.dry].data;
            filter_.wetFloat = kFilterMixDataLibrary[filter_.wet].data;
        }

        // update lcd
        if (((menu == FILTER_0_MENU) && (filterNum_ == 0)) || ((menu == FILTER_1_MENU) && (filterNum_ == 1))) {
            lcd_drawFilter_DryData(filterNum_);
            lcd_drawFilter_WetData(filterNum_);
        }
    }
}

void Controller::filter_genTransition(uint8_t filterNum_, FilterTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_) {
    Filter &filter_ = filter[filterNum_];
    FilterGenTransition &gTransition = filter_.genTransition;

    gTransition.mode = mode_;
    gTransition.phase = FIL_PHASE_A;

    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    gTransition.activeType = activeType_;
    gTransition.targetType = targetType_;

    switch (mode_) {
    case FIL_MODE_NONE:
        break;

    case FIL_MODE_ACTIVE:
        switch (targetActive_) {
        case true:
            gTransition.activeDry = 1.0;
            gTransition.targetDry = 0.0;

            gTransition.activeWet = 0.0;
            gTransition.targetWet = 1.0;

            filter_.calculateCoef();

            transitionShowFlag = 2;
            break;

        case false:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;

            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;

            transitionShowFlag = 1;
            break;
        }
        break;

    case FIL_MODE_TYPE:
        gTransition.activeDry = 0.0;
        gTransition.targetDry = 1.0;

        gTransition.activeWet = 1.0;
        gTransition.targetWet = 0.0;

        transitionShowFlag = 1;
        break;
    }

    filter_calculateGenTransition(filterNum_);
    gTransition.active = true;
}

void Controller::filter_mixTransition(uint8_t filterNum_, float dryFloat_, float wetFloat_) {
    Filter &filter_ = filter[filterNum_];
    FilterMixTransition &mTransition = filter_.mixTransition;

    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    filter_calculateMixTransition(filterNum_);
    mTransition.active = true;
}

void Controller::filter_calculateGenTransition(uint8_t filterNum_) {
    Filter &filter_ = filter[filterNum_];
    FilterGenTransition &gTransition = filter_.genTransition;

    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = FIL_ACTION_DOWN;
    }

    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = FIL_ACTION_DOWN;
    }
}

void Controller::filter_calculateMixTransition(uint8_t filterNum_) {
    Filter &filter_ = filter[filterNum_];
    FilterMixTransition &mTransition = filter_.mixTransition;

    if (filter_.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_NONE;
    } else if (filter_.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_UP;
    } else if (filter_.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = FIL_ACTION_DOWN;
    }

    if (filter_.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_NONE;
    } else if (filter_.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_UP;
    } else if (filter_.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = FIL_ACTION_DOWN;
    }
}

/* Envelope functions --------------------------------------------------------*/

void Controller::envelope_select() {
    if (menu != ENVELOPE_MENU) {
        preMenu = menu;
        menu = ENVELOPE_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEnvelopeMenu();
    }
}

void Controller::envelope_reset() {
    envelope_setActive(kInitialEnvelopeActive);
    envelope_setType(kInitialEnvelopeType);
    envelope_setCurve(kInitialEnvelopeCurve);
    envelope_setAttackTime(kInitialEnvelopeAttackTime);
    envelope_setDecayTime(kInitialEnvelopeDecayTime);
    envelope_setSustainLevel(kInitialEnvelopeSustainLevel);
    envelope_setReleaseTime(kInitialEnvelopeReleaseTime);
}

void Controller::envelope_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::envelope_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::envelope_menuUp() {
    switch (menuTab) {
    case 0:
        envelope_setActive(!envelope.active);
        break;

    case 2:
        if (envelope.type < kMaxEnvelopeType) {
            envelope_setType(envelope.type + 1);
        } else {
            envelope_setType(kMinEnvelopeType);
        }
        break;

    case 3:
        if (envelope.curve < kMaxEnvelopeCurve) {
            envelope_setCurve(envelope.curve + 1);
        } else {
            envelope_setCurve(kMinEnvelopeCurve);
        }
        break;

    case 4:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_ASR) || (envelope.type == ENV_AD)) {
            if (envelope.attackTime < kMaxEnvelopeTime) {
                envelope_setAttackTime(envelope.attackTime + 1);
            }
        }
        break;

    case 5:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_AD)) {
            if (envelope.decayTime < kMaxEnvelopeTime) {
                envelope_setDecayTime(envelope.decayTime + 1);
            }
        }
        break;

    case 6:
        if (envelope.type == ENV_ADSR) {
            if (envelope.sustainLevel < kMaxEnvelopeLevel) {
                envelope_setSustainLevel(envelope.sustainLevel + 1);
            }
        }
        break;

    case 7:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_ASR)) {
            if (envelope.releaseTime < kMaxEnvelopeTime) {
                envelope_setReleaseTime(envelope.releaseTime + 1);
            }
        }
        break;
    }
}

void Controller::envelope_menuDown() {
    uint8_t filterNum;
    (menu == FILTER_0_MENU) ? filterNum = 0 : filterNum = 1;

    switch (menuTab) {
    case 0:
        envelope_setActive(!envelope.active);
        break;

    case 2:
        if (envelope.type > kMinEnvelopeType) {
            envelope_setType(envelope.type - 1);
        } else {
            envelope_setType(kMaxEnvelopeType);
        }
        break;

    case 3:
        if (envelope.curve > kMinEnvelopeCurve) {
            envelope_setCurve(envelope.curve - 1);
        } else {
            envelope_setCurve(kMaxEnvelopeCurve);
        }
        break;

    case 4:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_ASR) || (envelope.type == ENV_AD)) {
            if (envelope.attackTime > kMinEnvelopeTime) {
                envelope_setAttackTime(envelope.attackTime - 1);
            }
        }
        break;

    case 5:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_AD)) {
            if (envelope.decayTime > kMinEnvelopeTime) {
                envelope_setDecayTime(envelope.decayTime - 1);
            }
        }
        break;

    case 6:
        if (envelope.type == ENV_ADSR) {
            if (envelope.sustainLevel > kMinEnvelopeLevel) {
                envelope_setSustainLevel(envelope.sustainLevel - 1);
            }
        }
        break;

    case 7:
        if ((envelope.type == ENV_ADSR) || (envelope.type == ENV_ASR)) {
            if (envelope.releaseTime > kMinEnvelopeTime) {
                envelope_setReleaseTime(envelope.releaseTime - 1);
            }
        }
        break;
    }
}

void Controller::envelope_setActive(bool active_) {
    // update data
    envelope.active = active_;
    // update lcd
    if (menu == ENVELOPE_MENU)
        lcd_drawEnvelope_ActiveData();
    lcd_drawInfo_Envelope_ActiveData();
    lcd_drawInfo_Envelope_TypeData();
}

void Controller::envelope_setType(uint8_t type_) {
    if ((type_ >= kMinEnvelopeType) && (type_ <= kMaxEnvelopeType)) {
        // update data
        envelope.type = type_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_TypeData();
            lcd_drawEnvelope_AttackTimeData();
            lcd_drawEnvelope_DecayTimeData();
            lcd_drawEnvelope_SustainLevelData();
            lcd_drawEnvelope_ReleaseTimeData();
        }
        lcd_drawInfo_Envelope_TypeData();
        lcd_drawInfo_Envelope_AttackTimeData();
        lcd_drawInfo_Envelope_DecayTimeData();
        lcd_drawInfo_Envelope_SustainLevelData();
        lcd_drawInfo_Envelope_ReleaseTimeData();
    }
}

void Controller::envelope_setCurve(uint8_t curve_) {
    if ((curve_ >= kMinEnvelopeCurve) && (curve_ <= kMaxEnvelopeCurve)) {
        // update data
        envelope.curve = curve_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_CurveData();
        }
        lcd_drawInfo_Envelope_CurveData();
    }
}

void Controller::envelope_setAttackTime(uint8_t time_) {
    if ((time_ >= kMinEnvelopeTime) && (time_ <= kMaxEnvelopeTime)) {
        // update data
        envelope.attackTime = time_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_AttackTimeData();
        }
        lcd_drawInfo_Envelope_AttackTimeData();
    }
}

void Controller::envelope_setDecayTime(uint8_t time_) {
    if ((time_ >= kMinEnvelopeTime) && (time_ <= kMaxEnvelopeTime)) {
        // update data
        envelope.decayTime = time_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_DecayTimeData();
        }
        lcd_drawInfo_Envelope_DecayTimeData();
    }
}

void Controller::envelope_setSustainLevel(uint8_t level_) {
    if ((level_ >= kMinEnvelopeLevel) && (level_ <= kMaxEnvelopeLevel)) {
        // update data
        envelope.sustainLevel = level_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_SustainLevelData();
        }
        lcd_drawInfo_Envelope_SustainLevelData();
    }
}

void Controller::envelope_setReleaseTime(uint8_t time_) {
    if ((time_ >= kMinEnvelopeTime) && (time_ <= kMaxEnvelopeTime)) {
        // update data
        envelope.releaseTime = time_;
        // update lcd
        if (menu == ENVELOPE_MENU) {
            lcd_drawEnvelope_ReleaseTimeData();
        }
        lcd_drawInfo_Envelope_ReleaseTimeData();
    }
}

/* Effect functions ----------------------------------------------------------*/

void Controller::effect_select() {
    if (menu == EFFECT_0_MENU) {
        preMenu = menu;
        menu = EFFECT_1_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEffectMenu(1);
    } else if (menu == EFFECT_1_MENU) {
        preMenu = menu;
        menu = EFFECT_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEffectMenu(0);
    } else {
        preMenu = menu;
        menu = EFFECT_0_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawEffectMenu(0);
    }
}

void Controller::effect_reset(uint8_t effectNum_) {
    effect_setActive(effectNum_, kInitialEffectActive);
    effect_setType(effectNum_, kInitialEffectType);

    effect[effectNum_].reset();
}

void Controller::effect_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        if (menuTab == 0) {
            menuTab += 2;
        } else {
            menuTab += 1;
        }
        lcd_transitionSelect();
    }
}

void Controller::effect_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        if (menuTab == 2) {
            menuTab -= 2;
        } else {
            menuTab -= 1;
        }
        lcd_transitionSelect();
    }
}

void Controller::effect_menuUp() {
    uint8_t effectNum_;
    (menu == EFFECT_0_MENU) ? effectNum_ = 0 : effectNum_ = 1;
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;
    SubEffect &subEffect_ = effect[effectNum_].subEffect[type_];

    switch (menuTab) {
    case 0:
        effect_setActive(effectNum_, !effect[effectNum_].active);
        break;

    case 2:
        if (effect[effectNum_].type < kMaxEffectType) {
            effect_setType(effectNum_, effect[effectNum_].type + 1);
        } else {
            effect_setType(effectNum_, kMinEffectType);
        }
        break;

    case 3:
        if (subEffect_.aData < subEffect_.kMaxAData) {
            effect_setAData(effectNum_, type_, subEffect_.aData + 1);
        }
        break;

    case 4:
        if (subEffect_.bData < subEffect_.kMaxBData) {
            effect_setBData(effectNum_, type_, subEffect_.bData + 1);
        }
        break;

    case 5:
        if (subEffect_.cData < subEffect_.kMaxCData) {
            effect_setCData(effectNum_, type_, subEffect_.cData + 1);
        }
        break;

    case 6:
        if (subEffect_.dData < subEffect_.kMaxDData) {
            effect_setDData(effectNum_, type_, subEffect_.dData + 1);
        } else if ((type_ != EF_COMPRESSOR) && (type_ != EF_EXPANDER) && (subEffect_.dData == subEffect_.kMaxDData) && (subEffect_.eData > subEffect_.kMinEData)) {
            effect_setEData(effectNum_, type_, subEffect_.eData - 1);
        }
        break;

    case 7:
        if (subEffect_.eData < subEffect_.kMaxEData) {
            effect_setEData(effectNum_, type_, subEffect_.eData + 1);
        } else if ((type_ != EF_COMPRESSOR) && (type_ != EF_EXPANDER) && (subEffect_.eData == subEffect_.kMaxEData) && (subEffect_.dData > subEffect_.kMinDData)) {
            effect_setDData(effectNum_, type_, subEffect_.dData - 1);
        }
        break;
    }
}

void Controller::effect_menuDown() {
    uint8_t effectNum_;
    (menu == EFFECT_0_MENU) ? effectNum_ = 0 : effectNum_ = 1;
    Effect &effect_ = effect[effectNum_];
    uint8_t type_ = effect[effectNum_].type;
    SubEffect &subEffect_ = effect[effectNum_].subEffect[type_];

    switch (menuTab) {
    case 0:
        effect_setActive(effectNum_, !effect[effectNum_].active);
        break;

    case 2:
        if (effect[effectNum_].type > kMinEffectType) {
            effect_setType(effectNum_, effect[effectNum_].type - 1);
        } else {
            effect_setType(effectNum_, kMaxEffectType);
        }
        break;

    case 3:
        if (subEffect_.aData > subEffect_.kMinAData) {
            effect_setAData(effectNum_, type_, subEffect_.aData - 1);
        }
        break;

    case 4:
        if (subEffect_.bData > subEffect_.kMinBData) {
            effect_setBData(effectNum_, type_, subEffect_.bData - 1);
        }
        break;

    case 5:
        if (subEffect_.cData > subEffect_.kMinCData) {
            effect_setCData(effectNum_, type_, subEffect_.cData - 1);
        }
        break;

    case 6:
        if (subEffect_.dData > subEffect_.kMinDData) {
            effect_setDData(effectNum_, type_, subEffect_.dData - 1);
        }
        break;

    case 7:
        if (subEffect_.eData > subEffect_.kMinEData) {
            effect_setEData(effectNum_, type_, subEffect_.eData - 1);
        }
        break;
    }
}

void Controller::effect_setActive(uint8_t effectNum_, bool active_) {
    Effect &effect_ = effect[effectNum_];

    if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (!effect_.genTransition.active)) {
        // update data
        if (active_)
            effect_cleanMemory(effectNum_, effect_.type);
        effect_genTransition(effectNum_, EF_MODE_ACTIVE, effect_.active, active_, effect_.type, effect_.type);
        effect_.active = active_;
        // update lcd
        lcd_drawEffect_ActiveData(effectNum_);
    } else if (!effect[effectNum_].genTransition.active) {
        // update data
        effect_.active = active_;
        // update lcd
        if (menu == MAIN_MENU) {
            switch (effectNum_) {
            case 0:
                lcd_drawMain_Effect0Data();
                break;

            case 1:
                lcd_drawMain_Effect1Data();
                break;
            }
        }
    }
}

void Controller::effect_setType(uint8_t effectNum_, uint8_t type_) {
    Effect &effect_ = effect[effectNum_];

    if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (!effect_.genTransition.active)) {
        // update data
        if (effect_.active) {
            effect_cleanMemory(effectNum_, type_);
            effect_genTransition(effectNum_, EF_MODE_TYPE, effect_.active, effect_.active, effect_.type, type_);
            effect_.type = type_;
        } else {
            effect_.type = type_;
        }
        // update lcd
        lcd_drawEffect_TypeData(effectNum_);
        lcd_drawEffect_AData(effectNum_);
        lcd_drawEffect_BData(effectNum_);
        lcd_drawEffect_CData(effectNum_);
        lcd_drawEffect_DData(effectNum_);
        lcd_drawEffect_EData(effectNum_);
    } else if (!effect[effectNum_].genTransition.active) {
        // update data
        effect_cleanMemory(effectNum_, type_);
        effect_.type = type_;
        // update lcd
        if (menu == MAIN_MENU) {
            switch (effectNum_) {
            case 0:
                lcd_drawMain_Effect0Data();
                break;

            case 1:
                lcd_drawMain_Effect1Data();
                break;
            }
        }
    }
}

void Controller::effect_setAData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t aData_) {
    Effect &effect_ = effect[effectNum_];
    SubEffect &subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    Delay &delay = effect_.delay;
    Chorus &chorus = effect_.chorus;
    Flanger &flanger = effect_.flanger;
    Phaser &phaser = effect_.phaser;
    Compressor &compressor = effect_.compressor;
    Expander &expander = effect_.expander;
    Overdrive &overdrive = effect_.overdrive;
    Distortion &distortion = effect_.distortion;
    Bitcrusher &bitcrusher = effect_.bitcrusher;

    if ((aData_ >= subEffect_.kMinAData) && (aData_ <= subEffect_.kMaxAData)) {
        // update data
        switch (subEffectNum_) {
        case EF_DELAY: // time
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.active) && (!effect_.genTransition.active)) {
                effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                subEffect_.aData = aData_;
                delay.aTime = aData_;
            } else if (!effect[effectNum_].genTransition.active) {
                subEffect_.aData = aData_;
                delay.aTime = aData_;
                delay.time = kDelayTimeDataLibrary[aData_].data;
                delay.update(rhythm.tempo);
            }
            break;

        case EF_CHORUS: // time
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.active) && (!effect_.genTransition.active)) {
                effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                subEffect_.aData = aData_;
                chorus.aTime = aData_;
            } else if (!effect[effectNum_].genTransition.active) {
                subEffect_.aData = aData_;
                chorus.aTime = aData_;
                chorus.time = kChorusTimeDataLibrary[aData_].data;
                chorus.update();
            }
            break;

        case EF_FLANGER: // time
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.active) && (!effect_.genTransition.active)) {
                effect_genTransition(effectNum_, EF_MODE_TIME, effect_.active, effect_.active, effect_.type, effect_.type);
                subEffect_.aData = aData_;
                flanger.aTime = aData_;
            } else if (!effect[effectNum_].genTransition.active) {
                subEffect_.aData = aData_;
                flanger.aTime = aData_;
                flanger.time = kFlangerTimeDataLibrary[aData_].data;
                flanger.update();
            }
            break;

        case EF_PHASER: // startFreq
            if (aData_ <= phaser.bEndFreq) {
                subEffect_.aData = aData_;
                phaser.aStartFreq = aData_;
                phaser.startFreq = kPhaserFreqDataLibrary[aData_].data;
                phaser.update();
            }
            break;

        case EF_COMPRESSOR: // threshold
            subEffect_.aData = aData_;
            compressor.aThreshold = aData_;
            compressor.threshold = kCompressorThresholdDataLibrary[aData_].data;
            compressor.update();
            break;

        case EF_EXPANDER: // threshold
            subEffect_.aData = aData_;
            expander.aThreshold = aData_;
            expander.threshold = kExpanderThresholdDataLibrary[aData_].data;
            expander.update();
            break;

        case EF_OVERDRIVE: // gain
            subEffect_.aData = aData_;
            overdrive.aGain = aData_;
            overdrive.gaindB = kOverdriveGainDataLibrary[aData_].data;
            overdrive.update();
            break;

        case EF_DISTORTION: // gain
            subEffect_.aData = aData_;
            distortion.aGain = aData_;
            distortion.gaindB = kDistortionGainDataLibrary[aData_].data;
            distortion.update();
            break;

        case EF_BITCRUSHER: // resolution
            subEffect_.aData = aData_;
            bitcrusher.aResolution = aData_;
            bitcrusher.resolution = kBitcrusherResolutionDataLibrary[aData_].data;
            bitcrusher.update();
            break;
        }
        // update lcd
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_AData(effectNum_);
        }
    }
}

void Controller::effect_setBData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t bData_) {
    Effect &effect_ = effect[effectNum_];
    SubEffect &subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    Delay &delay = effect_.delay;
    Chorus &chorus = effect_.chorus;
    Flanger &flanger = effect_.flanger;
    Phaser &phaser = effect_.phaser;
    Compressor &compressor = effect_.compressor;
    Expander &expander = effect_.expander;
    Overdrive &overdrive = effect_.overdrive;
    Distortion &distortion = effect_.distortion;
    Bitcrusher &bitcrusher = effect_.bitcrusher;

    if ((bData_ >= subEffect_.kMinBData) && (bData_ <= subEffect_.kMaxBData)) {
        // update data
        switch (subEffectNum_) {
        case EF_DELAY: // level
            subEffect_.bData = bData_;
            delay.bLevel = bData_;
            delay.level = kDelayLevelDataLibrary[bData_].data;
            break;

        case EF_CHORUS: // feedback
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.active) && (!effect_.genTransition.active)) {
                effect_genTransition(effectNum_, EF_MODE_FEEDBACK, effect_.active, effect_.active, effect_.type, effect_.type);
                subEffect_.bData = bData_;
                chorus.bFeedback = bData_;
            } else if (!effect[effectNum_].genTransition.active) {
                subEffect_.bData = bData_;
                chorus.bFeedback = bData_;
                chorus.feedback = kChorusFeedbackDataLibrary[bData_].data;
                chorus.update();
            }
            break;

        case EF_FLANGER: // feedback
            if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.active) && (!effect_.genTransition.active)) {
                effect_genTransition(effectNum_, EF_MODE_FEEDBACK, effect_.active, effect_.active, effect_.type, effect_.type);
                subEffect_.bData = bData_;
                flanger.bFeedback = bData_;
            } else if (!effect[effectNum_].genTransition.active) {
                subEffect_.bData = bData_;
                flanger.bFeedback = bData_;
                flanger.feedback = kFlangerFeedbackDataLibrary[bData_].data;
                flanger.update();
            }
            break;

        case EF_PHASER: // endFreq
            if (bData_ >= phaser.aStartFreq) {
                subEffect_.bData = bData_;
                phaser.bEndFreq = bData_;
                phaser.endFreq = kPhaserFreqDataLibrary[bData_].data;
                phaser.update();
            }
            break;

        case EF_COMPRESSOR: // rate
            subEffect_.bData = bData_;
            compressor.bRate = bData_;
            compressor.rate = kCompressorRateDataLibrary[bData_].data;
            compressor.update();
            break;

        case EF_EXPANDER: // rate
            subEffect_.bData = bData_;
            expander.bRate = bData_;
            expander.rate = kExpanderRateDataLibrary[bData_].data;
            expander.update();
            break;

        case EF_OVERDRIVE: // threshold
            subEffect_.bData = bData_;
            overdrive.bThreshold = bData_;
            overdrive.thresholddB = kOverdriveThresholdDataLibrary[bData_].data;
            overdrive.update();
            break;

        case EF_DISTORTION: // threshold
            subEffect_.bData = bData_;
            distortion.bThreshold = bData_;
            distortion.thresholddB = kDistortionThresholdDataLibrary[bData_].data;
            distortion.update();
            break;

        case EF_BITCRUSHER: // sampleRate
            subEffect_.bData = bData_;
            bitcrusher.bSampleRate = bData_;
            bitcrusher.sampleRate = kBitcrusherSampleRateDataLibrary[bData_].data;
            bitcrusher.update();
            break;
        }
        // update lcd
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) ||
             ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) &&
            (effect_.type == subEffectNum_)) {
            lcd_drawEffect_BData(effectNum_);
        }
    }
}

void Controller::effect_setCData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t cData_) {
    Effect &effect_ = effect[effectNum_];
    SubEffect &subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    Delay &delay = effect_.delay;
    Chorus &chorus = effect_.chorus;
    Flanger &flanger = effect_.flanger;
    Phaser &phaser = effect_.phaser;
    Compressor &compressor = effect_.compressor;
    Expander &expander = effect_.expander;
    Overdrive &overdrive = effect_.overdrive;
    Distortion &distortion = effect_.distortion;
    Bitcrusher &bitcrusher = effect_.bitcrusher;

    if ((cData_ >= subEffect_.kMinCData) && (cData_ <= subEffect_.kMaxCData)) {
        // update data
        switch (subEffectNum_) {
        case EF_DELAY: // feedback
            subEffect_.cData = cData_;
            delay.cFeedback = cData_;
            delay.feedback = kDelayFeedbackDataLibrary[cData_].data;
            break;

        case EF_CHORUS: // rate
            subEffect_.cData = cData_;
            chorus.cRate = cData_;
            chorus.rate = kChorusRateDataLibrary[cData_].data;
            chorus.update();
            break;

        case EF_FLANGER: // rate
            subEffect_.cData = cData_;
            flanger.cRate = cData_;
            flanger.rate = kFlangerRateDataLibrary[cData_].data;
            flanger.update();
            break;

        case EF_PHASER: // rate
            subEffect_.cData = cData_;
            phaser.cRate = cData_;
            phaser.rate = kPhaserRateDataLibrary[cData_].data;
            phaser.update();
            break;

        case EF_COMPRESSOR: // time
            subEffect_.cData = cData_;
            compressor.cAttackTime = cData_;
            compressor.attackTime = kCompressorAttackTimeDataLibrary[cData_].data;
            compressor.update();
            break;

        case EF_EXPANDER: // time
            subEffect_.cData = cData_;
            expander.cAttackTime = cData_;
            expander.attackTime = kExpanderAttackTimeDataLibrary[cData_].data;
            expander.update();
            break;

        case EF_OVERDRIVE: // tone
            subEffect_.cData = cData_;
            overdrive.cTone = cData_;
            overdrive.tone = kOverdriveToneDataLibrary[cData_].data;
            overdrive.update();
            break;

        case EF_DISTORTION: // tone
            subEffect_.cData = cData_;
            distortion.cTone = cData_;
            distortion.tone = kDistortionToneDataLibrary[cData_].data;
            distortion.update();
            break;

        case EF_BITCRUSHER: // clipThreshold
            subEffect_.cData = cData_;
            bitcrusher.cThreshold = cData_;
            bitcrusher.threshold = kBitcrusherThresholdDataLibrary[cData_].data;
            bitcrusher.update();
            break;
        }
        // update lcd
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) ||
             ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) &&
            (effect_.type == subEffectNum_)) {
            lcd_drawEffect_CData(effectNum_);
        }
    }
}

void Controller::effect_setDData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t dData_) {
    Effect &effect_ = effect[effectNum_];
    SubEffect &subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    Delay &delay = effect_.delay;
    Chorus &chorus = effect_.chorus;
    Flanger &flanger = effect_.flanger;
    Phaser &phaser = effect_.phaser;
    Compressor &compressor = effect_.compressor;
    Expander &expander = effect_.expander;
    Overdrive &overdrive = effect_.overdrive;
    Distortion &distortion = effect_.distortion;
    Bitcrusher &bitcrusher = effect_.bitcrusher;

    if ((dData_ >= subEffect_.kMinDData) && (dData_ <= subEffect_.kMaxDData) && (!effect_.mixTransition.active)) {
        // update data
        uint8_t targetDry;
        uint8_t targetWet;

        float targetDryFloat;
        float targetWetFloat;

        switch (subEffectNum_) {
        case EF_DELAY:
            targetDry = dData_;
            targetWet = delay.eWet;

            if ((delay.limitMix) && ((targetDry + targetWet) > delay.limitMixData)) {
                targetWet = delay.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            delay.dDry = targetDry;
            delay.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[delay.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[delay.eWet].data;
            break;

        case EF_CHORUS:
            targetDry = dData_;
            targetWet = chorus.eWet;

            if ((chorus.limitMix) && ((targetDry + targetWet) > chorus.limitMixData)) {
                targetWet = chorus.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            chorus.dDry = targetDry;
            chorus.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[chorus.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[chorus.eWet].data;
            break;

        case EF_FLANGER:
            targetDry = dData_;
            targetWet = flanger.eWet;

            if ((flanger.limitMix) && ((targetDry + targetWet) > flanger.limitMixData)) {
                targetWet = flanger.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            flanger.dDry = targetDry;
            flanger.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[flanger.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[flanger.eWet].data;
            break;

        case EF_PHASER:
            targetDry = dData_;
            targetWet = phaser.eWet;

            if ((phaser.limitMix) && ((targetDry + targetWet) > phaser.limitMixData)) {
                targetWet = phaser.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            phaser.dDry = targetDry;
            phaser.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[phaser.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[phaser.eWet].data;
            break;

        case EF_COMPRESSOR:
            subEffect_.dData = dData_;
            compressor.dReleaseTime = dData_;
            compressor.releaseTime = kCompressorReleaseTimeDataLibrary[compressor.dReleaseTime].data;
            compressor.update();
            break;

        case EF_EXPANDER:
            subEffect_.dData = dData_;
            expander.dReleaseTime = dData_;
            expander.releaseTime = kExpanderReleaseTimeDataLibrary[expander.dReleaseTime].data;
            expander.update();
            break;

        case EF_OVERDRIVE:
            targetDry = dData_;
            targetWet = overdrive.eWet;

            if ((overdrive.limitMix) && ((targetDry + targetWet) > overdrive.limitMixData)) {
                targetWet = overdrive.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            overdrive.dDry = targetDry;
            overdrive.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[overdrive.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[overdrive.eWet].data;
            break;

        case EF_DISTORTION:
            targetDry = dData_;
            targetWet = distortion.eWet;

            if ((distortion.limitMix) && ((targetDry + targetWet) > distortion.limitMixData)) {
                targetWet = distortion.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            distortion.dDry = targetDry;
            distortion.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[distortion.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[distortion.eWet].data;
            break;

        case EF_BITCRUSHER:
            targetDry = dData_;
            targetWet = bitcrusher.eWet;

            if ((bitcrusher.limitMix) && ((targetDry + targetWet) > bitcrusher.limitMixData)) {
                targetWet = bitcrusher.limitMixData - targetDry;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            bitcrusher.dDry = targetDry;
            bitcrusher.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[bitcrusher.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[bitcrusher.eWet].data;
            break;
        }

        if (effect_.active) {
            effect_mixTransition(effectNum_, targetDryFloat, targetWetFloat);
        } else {
            effect_.dryFloat = targetDryFloat;
            effect_.wetFloat = targetWetFloat;
        }
        // update lcd
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_DData(effectNum_);
            lcd_drawEffect_EData(effectNum_);
        }
    }
}

void Controller::effect_setEData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t eData_) {
    Effect &effect_ = effect[effectNum_];
    SubEffect &subEffect_ = effect[effectNum_].subEffect[subEffectNum_];

    Delay &delay = effect_.delay;
    Chorus &chorus = effect_.chorus;
    Flanger &flanger = effect_.flanger;
    Phaser &phaser = effect_.phaser;
    Compressor &compressor = effect_.compressor;
    Expander &expander = effect_.expander;
    Overdrive &overdrive = effect_.overdrive;
    Distortion &distortion = effect_.distortion;
    Bitcrusher &bitcrusher = effect_.bitcrusher;

    if ((eData_ >= subEffect_.kMinEData) && (eData_ <= subEffect_.kMaxEData)) {
        // update data
        uint8_t targetDry;
        uint8_t targetWet;

        float targetDryFloat;
        float targetWetFloat;

        switch (subEffectNum_) {
        case EF_DELAY:
            targetDry = delay.dDry;
            targetWet = eData_;

            if ((delay.limitMix) && ((targetDry + targetWet) > delay.limitMixData)) {
                targetDry = delay.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            delay.dDry = targetDry;
            delay.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[delay.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[delay.eWet].data;
            break;

        case EF_CHORUS:
            targetDry = chorus.dDry;
            targetWet = eData_;

            if ((chorus.limitMix) && ((targetDry + targetWet) > chorus.limitMixData)) {
                targetDry = chorus.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            chorus.dDry = targetDry;
            chorus.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[chorus.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[chorus.eWet].data;
            break;

        case EF_FLANGER:
            targetDry = flanger.dDry;
            targetWet = eData_;

            if ((flanger.limitMix) && ((targetDry + targetWet) > flanger.limitMixData)) {
                targetDry = flanger.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            flanger.dDry = targetDry;
            flanger.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[flanger.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[flanger.eWet].data;
            break;

        case EF_PHASER:
            targetDry = phaser.dDry;
            targetWet = eData_;

            if ((phaser.limitMix) && ((targetDry + targetWet) > phaser.limitMixData)) {
                targetDry = phaser.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            phaser.dDry = targetDry;
            phaser.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[phaser.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[phaser.eWet].data;
            break;

        case EF_COMPRESSOR:
            subEffect_.eData = eData_;
            compressor.eMix = eData_;

            targetDryFloat = kEffectMixDataLibrary[20 - compressor.eMix].data;
            targetWetFloat = kEffectMixDataLibrary[compressor.eMix].data;
            break;

        case EF_EXPANDER:
            subEffect_.eData = eData_;
            expander.eMix = eData_;

            targetDryFloat = kEffectMixDataLibrary[20 - expander.eMix].data;
            targetWetFloat = kEffectMixDataLibrary[expander.eMix].data;
            break;

        case EF_OVERDRIVE:
            targetDry = overdrive.dDry;
            targetWet = eData_;

            if ((overdrive.limitMix) && ((targetDry + targetWet) > overdrive.limitMixData)) {
                targetDry = overdrive.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            overdrive.dDry = targetDry;
            overdrive.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[overdrive.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[overdrive.eWet].data;
            break;

        case EF_DISTORTION:
            targetDry = distortion.dDry;
            targetWet = eData_;

            if ((distortion.limitMix) && ((targetDry + targetWet) > distortion.limitMixData)) {
                targetDry = distortion.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            distortion.dDry = targetDry;
            distortion.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[distortion.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[distortion.eWet].data;
            break;

        case EF_BITCRUSHER:
            targetDry = bitcrusher.dDry;
            targetWet = eData_;

            if ((bitcrusher.limitMix) && ((targetDry + targetWet) > bitcrusher.limitMixData)) {
                targetDry = bitcrusher.limitMixData - targetWet;
            }

            subEffect_.dData = targetDry;
            subEffect_.eData = targetWet;

            bitcrusher.dDry = targetDry;
            bitcrusher.eWet = targetWet;

            targetDryFloat = kEffectMixDataLibrary[bitcrusher.dDry].data;
            targetWetFloat = kEffectMixDataLibrary[bitcrusher.eWet].data;
            break;
        }

        if (effect_.active) {
            effect_mixTransition(effectNum_, targetDryFloat, targetWetFloat);
        } else {
            effect_.dryFloat = targetDryFloat;
            effect_.wetFloat = targetWetFloat;
        }

        // update lcd
        if ((((menu == EFFECT_0_MENU) && (effectNum_ == 0)) || ((menu == EFFECT_1_MENU) && (effectNum_ == 1))) && (effect_.type == subEffectNum_)) {
            lcd_drawEffect_DData(effectNum_);
            lcd_drawEffect_EData(effectNum_);
        }
    }
}

void Controller::effect_genTransition(uint8_t effectNum_, EffectTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_) {
    Effect &effect_ = effect[effectNum_];
    EffectGenTransition &gTransition = effect_.genTransition;

    gTransition.mode = mode_;
    gTransition.phase = EF_PHASE_A;

    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    gTransition.activeType = activeType_;
    gTransition.targetType = targetType_;

    switch (mode_) {
    case EF_MODE_NONE:
        break;

    case EF_MODE_ACTIVE:
        switch (targetActive_) {
        case true:
            gTransition.activeDry = 1.0;
            gTransition.targetDry = 0.0;

            gTransition.activeWet = 0.0;
            gTransition.targetWet = 1.0;

            if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
                gTransition.activeRecordWet = 0.0;
                gTransition.targetRecordWet = 1.0;
            }
            transitionShowFlag = 2;
            break;

        case false:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;

            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;

            if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
                gTransition.activeRecordWet = 1.0;
                gTransition.targetRecordWet = 0.0;
            }
            transitionShowFlag = 1;
            break;
        }
        break;

    case EF_MODE_TYPE:
    case EF_MODE_TIME:
    case EF_MODE_FEEDBACK:
        gTransition.activeDry = 0.0;
        gTransition.targetDry = 1.0;

        gTransition.activeWet = 1.0;
        gTransition.targetWet = 0.0;

        if ((activeType_ == EF_DELAY) || (activeType_ == EF_CHORUS) || (activeType_ == EF_FLANGER)) {
            gTransition.activeRecordWet = 1.0;
            gTransition.targetRecordWet = 0.0;
        }

        transitionShowFlag = 1;
        break;
    }

    effect_calculateGenTransition(effectNum_);
    gTransition.active = true;
}

void Controller::effect_mixTransition(uint8_t effectNum_, float dryFloat_, float wetFloat_) {
    Effect &effect_ = effect[effectNum_];
    EffectMixTransition &mTransition = effect_.mixTransition;

    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    effect_calculateMixTransition(effectNum_);
    mTransition.active = true;
}

void Controller::effect_calculateGenTransition(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    EffectGenTransition &gTransition = effect_.genTransition;

    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = EF_ACTION_DOWN;
    }

    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = EF_ACTION_DOWN;
    }

    if (gTransition.activeRecordWet == gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_NONE;
    } else if (gTransition.activeRecordWet < gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_UP;
    } else if (gTransition.activeRecordWet > gTransition.targetRecordWet) {
        gTransition.actionRecordWet = EF_ACTION_DOWN;
    }
}

void Controller::effect_calculateMixTransition(uint8_t effectNum_) {
    Effect &effect_ = effect[effectNum_];
    EffectMixTransition &mTransition = effect_.mixTransition;

    if (effect_.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_NONE;
    } else if (effect_.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_UP;
    } else if (effect_.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = EF_ACTION_DOWN;
    }

    if (effect_.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_NONE;
    } else if (effect_.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_UP;
    } else if (effect_.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = EF_ACTION_DOWN;
    }
}

void Controller::effect_cleanMemory(uint8_t effectNum_, uint8_t type_) {
    Effect &effect_ = effect[effectNum_];

    switch (type_) {
    case EF_DELAY:
        effect_.delay.cleanMemory();
        memset((uint8_t *)effect_.delayAddress, 0x00, kDelayByteSize);
        break;

    case EF_CHORUS:
        effect_.chorus.cleanMemory();
        memset((uint8_t *)effect_.chorusAddress, 0x00, kChorusByteSize);
        break;

    case EF_FLANGER:
        effect_.flanger.cleanMemory();
        memset(effect_.flangerBuffer, 0x00, sizeof(effect_.flangerBuffer));
        break;

    case EF_PHASER:
        effect_.phaser.cleanMemory();
        memset(effect_.phaser.ff, 0x00, sizeof(effect_.phaser.ff));
        memset(effect_.phaser.fb, 0x00, sizeof(effect_.phaser.fb));
        break;

    case EF_COMPRESSOR:
        effect_.compressor.cleanMemory();
        break;

    case EF_EXPANDER:
        effect_.expander.cleanMemory();
        break;

    case EF_OVERDRIVE:
        effect_.overdrive.cleanMemory();
        break;

    case EF_DISTORTION:
        effect_.distortion.cleanMemory();
        break;

    case EF_BITCRUSHER:
        effect_.bitcrusher.cleanMemory();
        break;
    }
}

/* Reverb functions ----------------------------------------------------------*/

void Controller::reverb_select() {
    if (menu != REVERB_MENU) {
        preMenu = menu;
        menu = REVERB_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawReverbMenu();
    }
}

void Controller::reverb_reset() {
    reverb_setActive(kInitialReverbActive);
    reverb_setSize(kInitialReverbSize);
    reverb_setDecay(kInitialReverbDecay);
    reverb_setPreDelay(kInitialReverbPreDelay);
    reverb_setSurround(kInitialReverbSurround);
    reverb_setDry(kInitialReverbDry);
    reverb_setWet(kInitialReverbWet);
}

void Controller::reverb_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::reverb_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::reverb_menuUp() {
    switch (menuTab) {
    case 0:
        reverb_setActive(!reverb.active);
        break;

    case 2:
        if (reverb.size < kMaxReverbSize) {
            reverb_setSize(reverb.size + 1);
        }
        break;

    case 3:
        if (reverb.decay < kMaxReverbDecay) {
            reverb_setDecay(reverb.decay + 1);
        }
        break;

    case 4:
        if (reverb.preDelay < kMaxReverbPreDelay) {
            reverb_setPreDelay(reverb.preDelay + 1);
        }
        break;

    case 5:
        if (reverb.surround < kMaxReverbSurround) {
            reverb_setSurround(reverb.surround + 1);
        }
        break;

    case 6:
        if (reverb.dry < kMaxReverbDry) {
            reverb_setDry(reverb.dry + 1);
        } else if ((reverb.dry == kMaxReverbDry) && (reverb.wet > kMinReverbWet)) {
            reverb_setWet(reverb.wet - 1);
        }
        break;

    case 7:
        if (reverb.wet < kMaxReverbWet) {
            reverb_setWet(reverb.wet + 1);
        } else if ((reverb.wet == kMaxReverbWet) && (reverb.dry > kMinReverbDry)) {
            reverb_setDry(reverb.dry - 1);
        }
        break;
    }
}

void Controller::reverb_menuDown() {
    switch (menuTab) {
    case 0:
        reverb_setActive(!reverb.active);
        break;

    case 2:
        if (reverb.size > kMinReverbSize) {
            reverb_setSize(reverb.size - 1);
        }
        break;

    case 3:
        if (reverb.decay > kMinReverbDecay) {
            reverb_setDecay(reverb.decay - 1);
        }
        break;

    case 4:
        if (reverb.preDelay > kMinReverbPreDelay) {
            reverb_setPreDelay(reverb.preDelay - 1);
        }
        break;

    case 5:
        if (reverb.surround > kMinReverbSurround) {
            reverb_setSurround(reverb.surround - 1);
        }
        break;

    case 6:
        if (reverb.dry > kMinReverbDry) {
            reverb_setDry(reverb.dry - 1);
        }
        break;

    case 7:
        if (reverb.wet > kMinReverbWet) {
            reverb_setWet(reverb.wet - 1);
        }
        break;
    }
}

void Controller::reverb_setActive(bool active_) {
    if ((menu == REVERB_MENU) && (!reverb.genTransition.active)) {
        // update data
        // if (active_) reverb.cleanMemory();
        reverb_genTransition(REV_MODE_ACTIVE, reverb.active, active_);
        reverb.active = active_;
        // update lcd
        lcd_drawReverb_ActiveData();
    } else if (!reverb.genTransition.active) {
        // update data
        reverb.active = active_;
    }
}

void Controller::reverb_setSize(uint8_t size_) {
    if ((size_ >= kMinReverbSize) && (size_ <= kMaxReverbSize)) {
        // update data
        reverb.size = size_;
        reverb.setSize(kReverbSizeDataLibrary[size_].data);
        // update lcd
        if (menu == REVERB_MENU)
            lcd_drawReverb_SizeData();
    }
}

void Controller::reverb_setDecay(uint8_t decay_) {
    if ((decay_ >= kMinReverbDecay) && (decay_ <= kMaxReverbDecay)) {
        // update data
        reverb.decay = decay_;
        reverb.setDecay(kReverbDecayDataLibrary[decay_].data);
        // update lcd
        if (menu == REVERB_MENU)
            lcd_drawReverb_DecayData();
    }
}

void Controller::reverb_setPreDelay(uint8_t preDelay_) {
    if ((preDelay_ >= kMinReverbPreDelay) && (preDelay_ <= kMaxReverbPreDelay)) {
        if ((menu == REVERB_MENU) && (!reverb.genTransition.active)) {
            // update data
            if (reverb.active) {
                reverb.preDelay = preDelay_;
                reverb_genTransition(REV_MODE_PREDELAY, reverb.active, reverb.active);
            } else {
                reverb.preDelay = preDelay_;
                reverb.setPreDelay(kReverbPreDelayDataLibrary[preDelay_].data);
            }
            // update lcd
            lcd_drawReverb_PreDelayData();
        } else if (!reverb.genTransition.active) {
            // update data
            reverb.preDelay = preDelay_;
            reverb.setPreDelay(kReverbPreDelayDataLibrary[preDelay_].data);
        }
    }
}

void Controller::reverb_setSurround(uint8_t surround_) {
    if ((surround_ >= kMinReverbSurround) && (surround_ <= kMaxReverbSurround)) {
        if ((menu == REVERB_MENU) && (!reverb.genTransition.active)) {
            // update data
            if (reverb.active) {
                reverb.surround = surround_;
                reverb_genTransition(REV_MODE_SURROUND, reverb.active, reverb.active);
            } else {
                reverb.surround = surround_;
                reverb.setSurround(kReverbSurroundDataLibrary[surround_].data);
            }
            // update lcd
            lcd_drawReverb_SurroundData();
        } else if (!reverb.genTransition.active) {
            // update data
            reverb.surround = surround_;
            reverb.setSurround(kReverbSurroundDataLibrary[surround_].data);
        }
    }
}

void Controller::reverb_setDry(uint8_t dry_) {
    if ((dry_ >= kMinReverbDry) && (dry_ <= kMaxReverbDry) &&
        (!reverb.mixTransition.active)) {
        // update data
        uint8_t targetDry = dry_;
        uint8_t targetWet = reverb.wet;

        if ((reverb.limitMix) && ((targetDry + targetWet) > reverb.limitMixData)) {
            targetWet = reverb.limitMixData - targetDry;
        }

        reverb.dry = targetDry;
        reverb.wet = targetWet;

        float targetDryFloat = kReverbMixDataLibrary[reverb.dry].data;
        float targetWetFloat = kReverbMixDataLibrary[reverb.wet].data;

        if (reverb.active) {
            reverb_mixTransition(targetDryFloat, targetWetFloat);
        } else {
            reverb.dryFloat = targetDryFloat;
            reverb.wetFloat = targetWetFloat;
        }

        // update lcd
        if (menu == REVERB_MENU) {
            lcd_drawReverb_DryData();
            lcd_drawReverb_WetData();
        }
    }
}

void Controller::reverb_setWet(uint8_t wet_) {
    if ((wet_ >= kMinReverbWet) && (wet_ <= kMaxReverbWet) &&
        (!reverb.mixTransition.active)) {
        // update data
        uint8_t targetDry = reverb.dry;
        uint8_t targetWet = wet_;

        if ((reverb.limitMix) && ((targetDry + targetWet) > reverb.limitMixData)) {
            targetDry = reverb.limitMixData - targetWet;
        }

        reverb.dry = targetDry;
        reverb.wet = targetWet;

        float targetDryFloat = kReverbMixDataLibrary[reverb.dry].data;
        float targetWetFloat = kReverbMixDataLibrary[reverb.wet].data;

        if (reverb.active) {
            reverb_mixTransition(targetDryFloat, targetWetFloat);
        } else {
            reverb.dryFloat = targetDryFloat;
            reverb.wetFloat = targetWetFloat;
        }

        // update lcd
        if (menu == REVERB_MENU) {
            lcd_drawReverb_DryData();
            lcd_drawReverb_WetData();
        }
    }
}

void Controller::reverb_genTransition(ReverbTransitionMode mode_, bool activeActive_, bool targetActive_) {
    ReverbGenTransition &gTransition = reverb.genTransition;

    gTransition.mode = mode_;
    gTransition.phase = REV_PHASE_A;

    gTransition.activeActive = activeActive_;
    gTransition.targetActive = targetActive_;

    switch (mode_) {
    case REV_MODE_NONE:
        break;

    case REV_MODE_ACTIVE:
        switch (targetActive_) {
        case true:
            gTransition.activeDry = 1.0;
            gTransition.targetDry = 0.0;

            gTransition.activeWet = 0.0;
            gTransition.targetWet = 1.0;

            transitionShowFlag = 2;
            break;

        case false:
            gTransition.activeDry = 0.0;
            gTransition.targetDry = 1.0;

            gTransition.activeWet = 1.0;
            gTransition.targetWet = 0.0;

            transitionShowFlag = 1;
            break;
        }
        break;

    case REV_MODE_PREDELAY:
    case REV_MODE_SURROUND:
        gTransition.activeDry = 0.0;
        gTransition.targetDry = 1.0;

        gTransition.activeWet = 1.0;
        gTransition.targetWet = 0.0;

        transitionShowFlag = 1;
        break;
    }

    reverb_calculateGenTransition();
    gTransition.active = true;
}

void Controller::reverb_mixTransition(float dryFloat_, float wetFloat_) {
    ReverbMixTransition &mTransition = reverb.mixTransition;

    mTransition.targetDry = dryFloat_;
    mTransition.targetWet = wetFloat_;

    reverb_calculateMixTransition();
    mTransition.active = true;
}

void Controller::reverb_calculateGenTransition() {
    ReverbGenTransition &gTransition = reverb.genTransition;

    if (gTransition.activeDry == gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_NONE;
    } else if (gTransition.activeDry < gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_UP;
    } else if (gTransition.activeDry > gTransition.targetDry) {
        gTransition.actionDry = REV_ACTION_DOWN;
    }

    if (gTransition.activeWet == gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_NONE;
    } else if (gTransition.activeWet < gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_UP;
    } else if (gTransition.activeWet > gTransition.targetWet) {
        gTransition.actionWet = REV_ACTION_DOWN;
    }
}

void Controller::reverb_calculateMixTransition() {
    ReverbMixTransition &mTransition = reverb.mixTransition;

    if (reverb.dryFloat == mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_NONE;
    } else if (reverb.dryFloat < mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_UP;
    } else if (reverb.dryFloat > mTransition.targetDry) {
        mTransition.actionDry = REV_ACTION_DOWN;
    }

    if (reverb.wetFloat == mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_NONE;
    } else if (reverb.wetFloat < mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_UP;
    } else if (reverb.wetFloat > mTransition.targetWet) {
        mTransition.actionWet = REV_ACTION_DOWN;
    }
}

/* Key functions -------------------------------------------------------------*/

void Controller::key_select() {
    if (menu != KEY_MENU) {
        preMenu = menu;
        menu = KEY_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawKeyMenu();
    }
}

void Controller::key_reset() {
    key_setNote(kInitialKeyNote);
    key_setArpeg(kInitialKeyArpeg);
    key_setRate(kInitialKeyRate);
    key_setOsc(kInitialKeyOsc);
    key_setChord(kInitialKeyChord);
    key_setOrder(kInitialKeyOrder);
    key_setOctave(kInitialKeyOctave);
}

void Controller::key_menuRight() {
    if (menuTab < kMaxMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 0) ? menuTab += 2 : menuTab += 1;
        lcd_transitionSelect();
    }
}

void Controller::key_menuLeft() {
    if (menuTab > kMinMenu8Tab) {
        preMenuTab = menuTab;
        (menuTab == 2) ? menuTab -= 2 : menuTab -= 1;
        lcd_transitionSelect();
    }
}

void Controller::key_menuUp() {
    switch (menuTab) {
    case 0:
        (key.note < kMaxKeyNote) ? key_setNote(key.note + 1) : key_setNote(kMinKeyNote);
        break;

    case 2:
        key_setArpeg(!key.arpeg);
        break;

    case 3:
        if (key.rate < kMaxKeyRate)
            key_setRate(key.rate + 1);
        break;

    case 4:
        (key.osc < kMaxKeyOsc) ? key_setOsc(key.osc + 1) : key_setOsc(kMinKeyOsc);
        break;

    case 5:
        switch (key.chordType) {
        case CHORD_3:
            (key.chord < kMaxKeyChord3) ? key_setChord(key.chord + 1) : key_setChord(kMinKeyChord3);
            break;

        case CHORD_4:
            (key.chord < kMaxKeyChord4) ? key_setChord(key.chord + 1) : key_setChord(kMinKeyChord4);
            break;
        }
        break;

    case 6:
        (key.order < kMaxKeyOrder) ? key_setOrder(key.order + 1) : key_setOrder(kMinKeyOrder);
        break;

    case 7:
        (key.octave < kMaxKeyOctave) ? key_setOctave(key.octave + 1) : key_setOctave(kMinKeyOctave);
        break;
    }
}

void Controller::key_menuDown() {
    switch (menuTab) {
    case 0:
        (key.note > kMinKeyNote) ? key_setNote(key.note - 1) : key_setNote(kMaxKeyNote);
        break;

    case 2:
        key_setArpeg(!key.arpeg);
        break;

    case 3:
        if (key.rate > kMinKeyRate)
            key_setRate(key.rate - 1);
        break;

    case 4:
        (key.osc > kMinKeyOsc) ? key_setOsc(key.osc - 1) : key_setOsc(kMaxKeyOsc);
        break;

    case 5:
        switch (key.chordType) {
        case CHORD_3:
            (key.chord > kMinKeyChord3) ? key_setChord(key.chord - 1) : key_setChord(kMaxKeyChord3);
            break;

        case CHORD_4:
            (key.chord > kMinKeyChord4) ? key_setChord(key.chord - 1) : key_setChord(kMaxKeyChord4);
            break;
        }

        break;

    case 6:
        (key.order > kMinKeyOrder) ? key_setOrder(key.order - 1) : key_setOrder(kMaxKeyOrder);
        break;

    case 7:
        (key.octave > kMinKeyOctave) ? key_setOctave(key.octave - 1) : key_setOctave(kMaxKeyOctave);
        break;
    }
}

void Controller::key_setNote(uint8_t note_) {
    if ((note_ >= kMinKeyNote) && (note_ <= kMaxKeyNote)) {
        // update data
        key.note = note_;
        // update lcd
        if (menu == KEY_MENU)
            lcd_drawKey_NoteData();
        lcd_drawInfo_Key_NoteData();
    }
}

void Controller::key_setArpeg(bool arpeg_) {
    // update data
    key.arpeg = arpeg_;
    key_calculateArpeg();
    // update lcd
    if (menu == KEY_MENU)
        lcd_drawKey_ArpegData();
    lcd_drawInfo_Key_ArpegData();
    lcd_drawInfo_Key_PatternData();
}

void Controller::key_setRate(uint8_t rate_) {
    if ((rate_ >= kMinKeyRate) && (rate_ <= kMaxKeyRate)) {
        // update data
        key.rate = rate_;
        switch (key.rate) {
        case 1: // 1/12
        case 3: // 1/6
        case 5: // 1/3
            if (key.chordType != CHORD_3) {
                key.chordType = CHORD_3;
                key_setChord(0);
            }
            break;

        case 0: // 1/16
        case 2: // 1/8
        case 4: // 1/4
        case 6: // 1/2
        case 7: // 1/1
            if (key.chordType != CHORD_4) {
                key.chordType = CHORD_4;
                key_setChord(0);
            }
            break;
        }

        key_calculateArpeg();
        // update lcd
        if (menu == KEY_MENU)
            lcd_drawKey_RateData();
        lcd_drawInfo_Key_RateData();
        lcd_drawInfo_Key_PatternData();
    }
}

void Controller::key_setOsc(uint8_t osc_) {
    if ((osc_ >= kMinKeyOsc) && (osc_ <= kMaxKeyOsc)) {
        // update data
        key.osc = osc_;
        switch (key.osc) {
        case 0:
            key.oscActive[0] = true;
            key.oscActive[1] = true;
            break;

        case 1:
            key.oscActive[0] = true;
            key.oscActive[1] = false;
            break;

        case 2:
            key.oscActive[0] = false;
            key.oscActive[1] = true;
            break;
        }
        // update lcd
        if (menu == KEY_MENU) {
            lcd_drawKey_OscData();
        }
        lcd_drawInfo_Key_OscData();
    }
}

void Controller::key_setChord(uint8_t chord_) {
    if (((key.chordType == CHORD_3) && (chord_ >= kMinKeyChord3) && (chord_ <= kMaxKeyChord3)) || ((key.chordType == CHORD_4) && (chord_ >= kMinKeyChord4) && (chord_ <= kMaxKeyChord4))) {
        // update data
        key.chord = chord_;
        key_calculateArpeg();
        // update lcd
        if (menu == KEY_MENU)
            lcd_drawKey_ChordData();
        lcd_drawInfo_Key_ChordData();
        lcd_drawInfo_Key_PatternData();
    }
}

void Controller::key_setOrder(uint8_t order_) {
    if ((order_ >= kMinKeyOrder) && (order_ <= kMaxKeyOrder)) {
        // update data
        key.order = order_;
        key_calculateArpeg();
        // update lcd
        if (menu == KEY_MENU)
            lcd_drawKey_OrderData();
        lcd_drawInfo_Key_OrderData();
        lcd_drawInfo_Key_PatternData();
    }
}

void Controller::key_setOctave(uint8_t octaveRange_) {
    if ((octaveRange_ >= kMinKeyOctave) && (octaveRange_ <= kMaxKeyOctave)) {
        // update data
        key.octave = octaveRange_;
        key_calculateArpeg();
        // update lcd
        if (menu == KEY_MENU)
            lcd_drawKey_OctaveData();
        lcd_drawInfo_Key_OctaveData();
        lcd_drawInfo_Key_PatternData();
    }
}

void Controller::key_calculateArpeg() {
    switch (key.chordType) {
    case CHORD_3:
        switch (key.octave) {
        case 0: // single
            switch (key.order) {
            case 0: // up
                key.chordPattern = 1;
                key.stepSize = 3;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][0][0];
                break;

            case 1: // down
                key.chordPattern = 1;
                key.stepSize = 3;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][1][0];
                break;

            case 2: // up-down
                key.chordPattern = 2;
                key.stepSize = 6;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][2][0];
                break;

            case 3: // down-up
                key.chordPattern = 2;
                key.stepSize = 6;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][3][0];
                break;

            case 4: // random
                key.chordPattern = 1;
                key.stepSize = 3;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][0][0];
                break;
            }
            break;

        case 1: // double
            switch (key.order) {
            case 0: // up
                key.chordPattern = 3;
                key.stepSize = 6;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][0][1];
                break;

            case 1: // down
                key.chordPattern = 3;
                key.stepSize = 6;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][1][1];
                break;

            case 2: // up-down
                key.chordPattern = 4;
                key.stepSize = 12;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][2][1];
                break;

            case 3: // down-up
                key.chordPattern = 4;
                key.stepSize = 12;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][3][1];
                break;

            case 4: // random
                key.chordPattern = 3;
                key.stepSize = 6;
                key.chordPointer = (uint8_t *)kKeyArpeg3DataLibrary[key.chord][0][1];
                break;
            }
            break;
        }
        break;

    case CHORD_4:
        switch (key.octave) {
        case 0: // single
            switch (key.order) {
            case 0: // up
                key.chordPattern = 5;
                key.stepSize = 4;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][0][0];
                break;

            case 1: // down
                key.chordPattern = 5;
                key.stepSize = 4;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][1][0];
                break;

            case 2: // up-down
                key.chordPattern = 6;
                key.stepSize = 8;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][2][0];
                break;

            case 3: // down-up
                key.chordPattern = 6;
                key.stepSize = 8;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][3][0];
                break;

            case 4: // random
                key.chordPattern = 5;
                key.stepSize = 4;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][0][0];
                break;
            }
            break;

        case 1: // double
            switch (key.order) {
            case 0: // up
                key.chordPattern = 7;
                key.stepSize = 8;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][0][1];
                break;

            case 1: // down
                key.chordPattern = 7;
                key.stepSize = 8;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][1][1];
                break;

            case 2: // up-down
                key.chordPattern = 8;
                key.stepSize = 16;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][2][1];
                break;

            case 3: // down-up
                key.chordPattern = 8;
                key.stepSize = 16;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][3][1];
                break;

            case 4: // random
                key.chordPattern = 7;
                key.stepSize = 8;
                key.chordPointer = (uint8_t *)kKeyArpeg4DataLibrary[key.chord][0][1];
                break;
            }
            break;
        }
        break;
    }
}

/* Song functions ------------------------------------------------------------*/

void Controller::song_select() {
    if (menu != SONG_MENU) {
        preMenu = menu;
        menu = SONG_MENU;
        preMenuTab = menuTab;
        menuTab = 0;
        subMenuTab = 0;
        preMenuSongClear();
        (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;
        lcd_transitionMenu();
        lcd_transitionSelect();
        lcd_drawSongMenu();
    }
}

void Controller::song_reset() {
    for (uint8_t i = 0; i < kBankLibrarySize; i++) {
        song_resetAllBeats(i);
    }
}

void Controller::song_menuRight() {
    Bank &bank = song.bankLibrary[activeBankNum];

    if (bank.lastActiveBeatNum > 0) {
        lcd_drawBeat(activeBankNum, selectedBeatNum, false);
        (selectedBeatNum < bank.lastActiveBeatNum) ? selectedBeatNum += 1 : selectedBeatNum = 0;
        // draw beat
        lcd_drawBeat(activeBankNum, selectedBeatNum, true);
        // draw layer menu data
        lcd_drawSong_BeatData();
        lcd_drawSong_BeatGraph();
    }
}

void Controller::song_menuLeft() {
    Bank &bank = song.bankLibrary[activeBankNum];

    if (bank.lastActiveBeatNum > 0) {
        lcd_drawBeat(activeBankNum, selectedBeatNum, false);
        (selectedBeatNum > 0) ? selectedBeatNum -= 1 : selectedBeatNum = bank.lastActiveBeatNum;
        // draw beat
        lcd_drawBeat(activeBankNum, selectedBeatNum, true);
        // draw layer menu data
        lcd_drawSong_BeatData();
        lcd_drawSong_BeatGraph();
    }
}

void Controller::song_menuUp() {
    Bank &bank = song.bankLibrary[activeBankNum];
    if (bank.lastActiveBeatNum > 0) {
        Beat &beat = bank.beatLibrary[selectedBeatNum];
        if (beat.note < 11) {
            song_setBeatNoteOctave(beat.note + 1, beat.octave);
        } else if (beat.octave < kMaxOctave) {
            song_setBeatNoteOctave(0, beat.octave + 1);
        }
    }
}

void Controller::song_menuDown() {
    Bank &bank = song.bankLibrary[activeBankNum];
    if (bank.lastActiveBeatNum > 0) {
        Beat &beat = bank.beatLibrary[selectedBeatNum];
        if (beat.note > 0) {
            song_setBeatNoteOctave(beat.note - 1, beat.octave);
        } else if (beat.octave > kMinOctave) {
            song_setBeatNoteOctave(11, beat.octave - 1);
        }
    }
}

void Controller::song_startRecordBeat(uint8_t bankNum_, uint16_t interval_, uint8_t octaveNum_, uint8_t noteNum_) {
    Bank &bank = song.bankLibrary[bankNum_];
    // end pre-record beat
    if ((recordBeatNum != -1) && (noteNum_ != recordNoteNum)) {
        song_endRecordBeat(bankNum_, interval_);
    }

    // start record beat
    if ((recordBeatNum == -1) && (bank.lastActiveBeatNum < kBeatLibrarySize)) {
        uint16_t quantizeInterval = kQuantizeInterval[rhythm.quantize];
        uint16_t startInterval = ((interval_ / quantizeInterval) + ((interval_ % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;

        if (startInterval != songInterval) {
            // limit previous beat's interval
            if (bank.lastActiveBeatNum != -1) {
                for (int8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                    Beat &beat = bank.beatLibrary[i];
                    if ((beat.startInterval < startInterval) && (beat.endInterval > startInterval)) {
                        lcd_clearBeatInterval(bankNum_, i, startInterval, beat.endInterval);
                        beat.endInterval = startInterval;
                        break;
                    }
                }
            }

            // calculate beatNum
            bool duplicate = false;
            uint8_t beatNum;
            if (bank.lastActiveBeatNum == -1) {
                beatNum = 0;
            } else {
                // check if there exists beat with same start interval
                for (int8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                    if ((bank.beatLibrary[i].active) && (bank.beatLibrary[i].startInterval == startInterval)) {
                        duplicate = true;
                        beatNum = i;
                        break;
                    }
                }
                if (duplicate) {
                    lcd_clearBeat(bankNum_, beatNum);
                    bank.beatLibrary[beatNum].reset();
                } else {
                    beatNum = 0;
                    // get beatNum
                    for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
                        if (bank.beatLibrary[i].startInterval < startInterval) {
                            beatNum = i + 1;
                        } else {
                            beatNum = i;
                            break;
                        }
                    }
                    // shift beats
                    if (beatNum <= bank.lastActiveBeatNum) {
                        for (int8_t j = bank.lastActiveBeatNum; j >= beatNum; j--) {
                            bank.beatLibrary[j + 1] = bank.beatLibrary[j];
                        }
                    }
                }
            }

            // set beat
            bank.beatLibrary[beatNum].start(startInterval, octaveNum_, noteNum_);
            if (menu == SONG_MENU) {
                /*
                if ((selectedBeatNum != -1) && (bank.lastActiveBeatNum != -1)) {
                    for (uint8_t i = 0; i < bank.lastActiveBeatNum; i++) {
                        lcd_drawBeat(bankNum_, i, false);
                    }
                }
                selectedBeatNum = beatNum;
                */
                lcd_drawStartBeat(bankNum_, beatNum, false);
            } else {
                lcd_drawStartBeat(bankNum_, beatNum, false);
            }

            song_calculateLastActiveBeatNum(bankNum_);
            song_calculateNextBeatNum(bankNum_, playInterval);
            recordNoteNum = noteNum_;
            recordBeatNum = beatNum;
            // remove double beat sound
            int8_t nextBeatNum = bank.nextBeatNum;
            if (nextBeatNum == beatNum) {
                (nextBeatNum < bank.lastActiveBeatNum) ? bank.nextBeatNum = nextBeatNum + 1 : bank.nextBeatNum = 0;
            }
        }
    }
}

void Controller::song_endRecordBeat(uint8_t bankNum_, uint16_t interval_) {
    Bank &bank = song.bankLibrary[bankNum_];
    // end record beat
    if (recordBeatNum != -1) {
        uint8_t beatNum = recordBeatNum;
        uint16_t quantizeInterval = kQuantizeInterval[rhythm.quantize];
        uint16_t startInterval = bank.beatLibrary[beatNum].startInterval;
        uint16_t endInterval = ((interval_ / quantizeInterval) + ((interval_ % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;

        if ((endInterval == startInterval) && ((endInterval + quantizeInterval) <= songInterval))
            endInterval += quantizeInterval;
        if (endInterval >= songInterval)
            endInterval = songInterval - 1;

        // limit next beat's interval
        for (uint8_t i = beatNum; i <= bank.lastActiveBeatNum; i++) {
            Beat &beat = bank.beatLibrary[i];
            if ((beat.startInterval > startInterval) && (beat.startInterval < endInterval)) {
                if (beat.endInterval > endInterval) {
                    lcd_clearBeatInterval(bankNum_, i, beat.startInterval, endInterval);
                    beat.startInterval = endInterval;
                    lcd_drawBeat(bankNum_, i, false);
                } else {
                    lcd_clearBeat(bankNum_, i);
                    bank.beatLibrary[i].reset();
                }
            }
        }

        // set beat
        bank.beatLibrary[beatNum].end(endInterval);
        song_arrangeActiveBeats(bankNum_, true, true, true);
        recordNoteNum = -1;
        recordBeatNum = -1;
        // draw beat end
        if (menu == SONG_MENU) {
            if ((selectedBeatNum >= beatNum) && ((selectedBeatNum + 1) < kBeatLibrarySize) && (bank.beatLibrary[selectedBeatNum + 1].active))
                selectedBeatNum += 1;
            if (selectedBeatNum != -1) {
                lcd_drawBeat(bankNum_, selectedBeatNum, false);
            }
            selectedBeatNum = beatNum;
            lcd_drawEndBeat(bankNum_, beatNum, true);
            lcd_drawSong_BeatData();
            lcd_drawSong_BeatGraph();
        } else {
            lcd_drawEndBeat(bankNum_, beatNum, false);
        }
    }
}

void Controller::song_setBeat(uint8_t bankNum_, uint16_t startInterval_, uint16_t endInterval_, uint8_t octave_, uint8_t note_) {
    song_startRecordBeat(bankNum_, startInterval_, octave_, note_);
    song_endRecordBeat(bankNum_, endInterval_);
}

void Controller::song_setBeatNoteOctave(uint8_t note_, uint8_t octave_) {
    if ((note_ >= 0) && (note_ <= 12) && (octave_ >= kMinOctave) && (octave_ <= kMaxOctave)) {
        Beat &beat = song.bankLibrary[activeBankNum].beatLibrary[selectedBeatNum];
        lcd_clearBeat(activeBankNum, selectedBeatNum);
        beat.note = note_;
        beat.octave = octave_;
        lcd_drawSong_BeatData();
        lcd_drawBeat(activeBankNum, selectedBeatNum, true);
    }
}

void Controller::song_generateBeat(uint8_t type) {
    uint16_t startInterval;
    uint16_t endInterval;
    switch (type) {
    case 0:
        for (uint8_t i = 0; i < rhythm.bar; i++) {
            startInterval = i * barInterval;
            endInterval = (i + 1) * barInterval;
            song_setBeat(activeBankNum, startInterval, endInterval, activeOctaveNum, 0);
        }
        break;

    case 1:
        for (uint8_t i = 0; i < rhythm.bar; i++) {
            startInterval = i * barInterval + (barInterval / 2);
            endInterval = (i + 1) * barInterval;
            song_setBeat(activeBankNum, startInterval, endInterval, activeOctaveNum, 0);
        }
        break;

    case 2:
        for (uint8_t i = 0; i < rhythm.measureTotal; i++) {
            startInterval = i * measureInterval;
            endInterval = (i + 1) * measureInterval;
            if (startInterval % (barInterval / 2) != 0) {
                song_setBeat(activeBankNum, startInterval, endInterval, activeOctaveNum, 0);
            }
        }
        break;
    }

    if (menu == SONG_MENU) {
        lcd_drawBeat(activeBankNum, selectedBeatNum, false);
        selectedBeatNum = 0;
        lcd_drawBeat(activeBankNum, selectedBeatNum, true);
        lcd_drawSong_BeatData();
        lcd_drawSong_BeatGraph();
    }

    song_calculateNextBeatNum(activeBankNum, playInterval);
}

void Controller::song_resetSelectedBeat() {
    Bank &bank = song.bankLibrary[activeBankNum];
    if ((bank.lastActiveBeatNum != -1) && (selectedBeatNum != -1)) {
        // stop play beat
        if ((playActive) && (bank.playBeatNum == selectedBeatNum)) {
            sD.releaseData.active = true;
            sD.releaseData.beatNum = sD.activeBeatNum;
        }
        // reset beat
        lcd_clearBeat(activeBankNum, selectedBeatNum);
        bank.beatLibrary[selectedBeatNum].reset();
        // arrange beat library
        song_arrangeActiveBeats(activeBankNum, false, true, true);
        // update view
        if (bank.lastActiveBeatNum != -1) {
            // set selectedBeatNum
            (selectedBeatNum == 0) ? selectedBeatNum = bank.lastActiveBeatNum : selectedBeatNum = selectedBeatNum - 1;
            // draw selected beat
            lcd_drawBeat(activeBankNum, selectedBeatNum, true);
            // draw song menu data
            lcd_drawSong_BeatData();
            lcd_drawSong_BeatGraph();
        } else {
            selectedBeatNum = -1;
            // draw song menu data
            lcd_drawSong_BeatData();
            lcd_drawSong_BeatGraph();
        }
    }
}

void Controller::song_resetBeat(uint8_t bankNum_, uint8_t beatNum_) {
    if ((bankNum_ == activeBankNum) && (beatNum_ == selectedBeatNum)) {
        song_resetSelectedBeat();
    } else {
        Bank &bank = song.bankLibrary[bankNum_];
        if ((bank.lastActiveBeatNum != -1) && (beatNum_ != -1)) {
            // reset beat
            lcd_clearBeat(bankNum_, beatNum_);
            bank.beatLibrary[beatNum_].reset();
            // arrange beat library
            song_arrangeActiveBeats(bankNum_, false, true, true);
        }
    }
}

void Controller::song_resetBeats(uint8_t bankNum_, uint16_t startInterval_) {
    Bank &bank = song.bankLibrary[bankNum_];
    if (bank.lastActiveBeatNum != -1) {
        bool reorder = false;
        for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
            Beat &beat = bank.beatLibrary[i];
            uint16_t beatStartInterval = beat.startInterval;
            uint16_t beatEndInterval = beat.endInterval;
            if (beatStartInterval >= startInterval_) {
                beat.reset();
                reorder = true;
                lcd_clearBeat(bankNum_, i);
            } else if ((beatStartInterval < startInterval_) && (beatEndInterval > startInterval_)) {
                beat.endInterval = startInterval_;
                lcd_clearBeat(bankNum_, i);
                lcd_drawBeat(bankNum_, i, false);
            }
        }
        song_arrangeActiveBeats(bankNum_, reorder, reorder, reorder);
    }
}

void Controller::song_resetAllBeats(uint8_t bankNum_) {
    Bank &bank = song.bankLibrary[bankNum_];
    if (bank.lastActiveBeatNum != -1) {
        // stop play beat
        if (playActive) {
            sD.releaseData.active = true;
            sD.releaseData.beatNum = sD.activeBeatNum;
        }
        // reset beats
        for (uint16_t i = 0; i < kBeatLibrarySize; i++) {
            bank.beatLibrary[i].reset();
        }
        bank.lastActiveBeatNum = -1;
        bank.nextBeatNum = -1;
        // update view
        if (bankNum_ == activeBankNum) {
            lcd_clearSong();
            if (menu == SONG_MENU) {
                selectedBeatNum = -1;
                lcd_drawSong_BeatData();
                lcd_drawSong_BeatGraph();
            }
        }
    }
}

void Controller::song_quantizeActiveBeats(uint8_t bankNum_) {
    Bank &bank = song.bankLibrary[bankNum_];
    uint16_t quantizeInterval = kQuantizeInterval[rhythm.quantize];
    if (bank.lastActiveBeatNum != -1) {
        for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
            Beat &beat = bank.beatLibrary[i];
            if (beat.active) {
                uint16_t startInterval = beat.startInterval;
                uint16_t endInterval = beat.endInterval;
                if (startInterval != 0)
                    startInterval -= 1;
                if (endInterval != 0)
                    endInterval -= 1;
                startInterval = ((startInterval / quantizeInterval) + ((startInterval % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;
                endInterval = ((endInterval / quantizeInterval) + ((endInterval % quantizeInterval) / (quantizeInterval / 2))) * quantizeInterval;
                if (startInterval == songInterval) {
                    beat.reset();
                } else if (endInterval == startInterval) {
                    endInterval += quantizeInterval;
                    if (endInterval > songInterval)
                        endInterval = songInterval;
                }
                if (beat.active) {
                    beat.startInterval = startInterval;
                    beat.endInterval = endInterval;
                }
            }
        }
        song_arrangeActiveBeats(bankNum_, true, true, true);
    }
}

void Controller::song_arrangeActiveBeats(uint8_t bankNum_, bool duplicate_, bool collect_, bool sort_) {
    Bank &bank = song.bankLibrary[bankNum_];
    if (bank.lastActiveBeatNum != -1) {
        // phase 01: reset duplicate active beats
        if (duplicate_) {
            for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
                Beat &beat = bank.beatLibrary[i];
                if (beat.active) {
                    uint16_t beatStartInterval = beat.startInterval;
                    for (uint8_t j = i + 1; j < kBeatLibrarySize; j++) {
                        Beat &nextBeat = bank.beatLibrary[j];
                        if (nextBeat.active) {
                            uint16_t nextStartInterval = nextBeat.startInterval;
                            if (nextStartInterval == beatStartInterval) {
                                nextBeat.reset();
                            }
                        }
                    }
                }
            }
        }

        // phase 02: collect active beats
        if (collect_) {
            Beat tempLibrary[kBeatLibrarySize];
            int8_t tempCounter = -1;
            for (uint8_t i = 0; i < kBeatLibrarySize; i++) {
                if (bank.beatLibrary[i].active) {
                    tempCounter += 1;
                    tempLibrary[tempCounter] = bank.beatLibrary[i];
                }
            }
            for (uint8_t j = 0; j < kBeatLibrarySize; j++) {
                bank.beatLibrary[j] = tempLibrary[j];
            }
            bank.lastActiveBeatNum = tempCounter;
        }

        // phase 03: sort active beats
        if ((sort_) && (bank.lastActiveBeatNum > 0)) {
            uint8_t lastActiveBeatNum = bank.lastActiveBeatNum;
            // a. sort ref start interval
            uint8_t min;
            Beat temp;
            for (uint8_t i = 0; i < (lastActiveBeatNum - 1); i++) {
                min = i;
                for (uint8_t j = i + 1; j < lastActiveBeatNum; j++) {
                    if (bank.beatLibrary[j].startInterval < bank.beatLibrary[min].startInterval) {
                        min = j;
                    }
                }
                temp = bank.beatLibrary[i];
                bank.beatLibrary[i] = bank.beatLibrary[min];
                bank.beatLibrary[min] = temp;
            }
            // b. set end interval
            for (uint8_t k = 0; k < lastActiveBeatNum; k++) {
                if (bank.beatLibrary[k].endInterval > bank.beatLibrary[k + 1].startInterval) {
                    bank.beatLibrary[k].endInterval = bank.beatLibrary[k + 1].startInterval;
                }
            }
            if (bank.beatLibrary[lastActiveBeatNum].endInterval > songInterval) {
                bank.beatLibrary[lastActiveBeatNum].endInterval = songInterval;
            }
        }

        // phase 04: calculate play beat
        song_calculateNextBeatNum(bankNum_, playInterval);
    }
}

void Controller::song_calculateLastActiveBeatNum(uint8_t bankNum_) {
    Bank &bank = song.bankLibrary[bankNum_];
    int8_t lastActiveBeatNum = -1;
    for (uint16_t i = 0; i < kBeatLibrarySize; i++) {
        if (bank.beatLibrary[i].active) {
            lastActiveBeatNum = i;
        } else {
            break;
        }
    }
    bank.lastActiveBeatNum = lastActiveBeatNum;
}

void Controller::song_calculateNextBeatNum(uint8_t bankNum_, uint16_t playInterval_) {
    Bank &bank = song.bankLibrary[bankNum_];
    if (bank.lastActiveBeatNum != -1) {
        for (uint8_t i = 0; i <= bank.lastActiveBeatNum; i++) {
            if ((bank.beatLibrary[i].active) && (bank.beatLibrary[i].startInterval >= playInterval)) {
                bank.nextBeatNum = i;
                break;
            }
        }
    } else {
        bank.nextBeatNum = -1;
    }
}

/* Bank functions ------------------------------------------------------------*/

void Controller::bank_select(uint8_t bankNum_) {
    if (bankNum_ != activeBankNum) {
        uint8_t preBankNum = activeBankNum;
        activeBankNum = bankNum_;
        lcd_drawBank(bankNum_, false);

        lcd_clearSong();
        lcd_drawSong(activeBankNum);
        song_calculateNextBeatNum(activeBankNum, playInterval);

        if (menu == SONG_MENU) {
            (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;
            if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)
                lcd_drawBeat(activeBankNum, 0, true);
            lcd_drawSong_BeatData();
            lcd_drawSong_BeatGraph();
        }
    }
}

void Controller::bank_trigger(uint8_t bankNum_) {
    if (bankNum_ != activeBankNum) {
        bankShiftFlag = true;
        targetBankNum = bankNum_;
        lcd_drawBank(bankNum_, true);
    } else if ((bankNum_ == activeBankNum) && (bankShiftFlag) && (targetBankNum != activeBankNum)) {
        bankShiftFlag = false;
        targetBankNum = bankNum_;
        lcd_drawBank(bankNum_, false);
    }
}

/* Note functions ------------------------------------------------------------*/

void Controller::pressNote(uint8_t octaveNum_, uint8_t noteNum_) {
    sD.startData.noteData = (octaveNum_ * 12) + (key.note + noteNum_ - 1);

    // play note
    switch (sD.activeBeatNum) {
    case -1:
        sD.startData.active = true;
        sD.startData.beatNum = 0;
        break;

    case 0:
        sD.startData.active = true;
        sD.startData.beatNum = 1;

        sD.endData.active = true;
        sD.endData.beatNum = 0;
        break;

    case 1:
        sD.startData.active = true;
        sD.startData.beatNum = 0;

        sD.endData.active = true;
        sD.endData.beatNum = 1;
        break;
    }

    notePressed = true;
    notePressedBeatNum = sD.startData.beatNum;

    // record note
    if (recordActive) {
        song_startRecordBeat(activeBankNum, playInterval, octaveNum_, noteNum_);
    }
}

void Controller::releaseNote() {
    if ((notePressedBeatNum != -1) && (sD.beatPlayData[notePressedBeatNum].active)) {
        // release note
        sD.releaseData.active = true;
        sD.releaseData.beatNum = notePressedBeatNum;
    }

    notePressed = false;
    notePressedBeatNum = -1;

    // record note
    if (recordActive)
        song_endRecordBeat(activeBankNum, playInterval);
}

/* Octave functions ----------------------------------------------------------*/

void Controller::octaveUp() {
    if (activeOctaveNum < kMaxOctave)
        setOctave(activeOctaveNum + 1);
}

void Controller::octaveDown() {
    if (activeOctaveNum > kMinOctave)
        setOctave(activeOctaveNum - 1);
}

void Controller::setOctave(uint8_t octave_) {
    if ((octave_ >= kMinOctave) && (octave_ <= kMaxOctave)) {
        activeOctaveNum = octave_;
        lcd_drawOctave();
    }
}

/* Play functions ------------------------------------------------------------*/

void Controller::record() {
    if (system.sync.syncOutPlay)
        sendSyncCommand(SYNC_RECORD);

    // start play & record
    if ((!playActive) && (!recordActive)) {
        if ((metronome.active) && (metronome.precount)) {
            metronome.precountState = true;
            metronome.precounter = 0;
            metronome.countDown = rhythm.measure;
        }
        recordActive = true;

        recordIcon.flag = true;
        recordIcon.mode = true;

        stopFlag = false;
        resetFlag = false;
        playActive = true;

        resetIcon.flag = true;
        resetIcon.mode = false;

        playIcon.flag = true;
        playIcon.mode = true;

        stopIcon.flag = true;
        stopIcon.mode = false;

        startPlayTimer();
    }

    // continue play & stop record
    else if ((playActive) && (recordActive)) {
        if (metronome.precountState) {
            stop();
        } else {
            recordActive = false;

            stopFlag = false;
            resetFlag = false;

            recordIcon.flag = true;
            recordIcon.mode = false;
        }
    }

    // continue play & start record
    else if ((playActive) && (!recordActive)) {
        recordActive = true;

        stopFlag = false;
        resetFlag = false;

        recordIcon.flag = true;
        recordIcon.mode = true;
    }
}

void Controller::play() {
    if (system.sync.syncOutPlay)
        sendSyncCommand(SYNC_PLAY);

    stopFlag = false;
    resetFlag = false;
    playActive = true;

    resetIcon.flag = true;
    resetIcon.mode = false;

    playIcon.flag = true;
    playIcon.mode = true;

    stopIcon.flag = true;
    stopIcon.mode = false;

    startPlayTimer();
}

void Controller::stop() {
    if ((system.sync.syncOutPlay) && (!stopFlag))
        sendSyncCommand(SYNC_STOP);

    if (metronome.precountState) {
        metronome.precountState = false;
        metronome.precounter = 0;
        metronome.countDownFlag = false;
        metronome.countDown = 0;
        lcd_clearCountDown();
    }

    if (bankShiftFlag) {
        bankShiftFlag = false;
        targetBankNum = activeBankNum;
        lcd_drawBank(activeBankNum, false);
    }

    if (sD.activeBeatNum != -1) {
        sD.releaseData.active = true;
        sD.releaseData.beatNum = sD.activeBeatNum;
    }

    stopFlag = false;
    resetFlag = false;

    playActive = false;
    recordActive = false;

    resetIcon.flag = true;
    resetIcon.mode = false;

    playIcon.flag = true;
    playIcon.mode = false;

    stopIcon.flag = true;
    stopIcon.mode = true;

    recordIcon.flag = true;
    recordIcon.mode = false;

    stopPlayTimer();
}

void Controller::reset() {
    if ((system.sync.syncOutPlay) && (!resetFlag))
        sendSyncCommand(SYNC_RESET);

    if ((playInterval != 0) || (metronome.precountState)) {
        if (metronome.precountState) {
            metronome.precountState = false;
            metronome.precounter = 0;
            metronome.countDownFlag = false;
            metronome.countDown = 0;
            lcd_clearCountDown();
        }

        if (bankShiftFlag) {
            bankShiftFlag = false;
            targetBankNum = activeBankNum;
            lcd_drawBank(activeBankNum, false);
        }

        if (sD.activeBeatNum != -1) {
            sD.releaseData.active = true;
            sD.releaseData.beatNum = sD.activeBeatNum;
        }

        stopFlag = false;
        resetFlag = false;

        playActive = false;
        recordActive = false;

        resetIcon.flag = true;
        resetIcon.mode = true;

        playIcon.flag = true;
        playIcon.mode = false;

        stopIcon.flag = true;
        stopIcon.mode = false;

        recordIcon.flag = true;
        recordIcon.mode = false;

        stopPlayTimer();

        playInterval = 0;

        resetPlayFlag = true;

        song_calculateNextBeatNum(activeBankNum, playInterval);
    }
}

void Controller::triggerStop() {
    if (system.sync.syncOutPlay)
        sendSyncCommand(SYNC_TRIG_STOP);

    if (metronome.precountState) {
        stop();
    } else {
        stopFlag = true;
        resetFlag = false;
        stopInterval = calculateTriggerInterval();

        resetIcon.flag = true;
        resetIcon.mode = false;

        // stopIcon.flag = true;
        // stopIcon.mode = true;
    }
}

void Controller::triggerReset() {
    if (system.sync.syncOutPlay)
        sendSyncCommand(SYNC_TRIG_RESET);

    if (metronome.precountState) {
        reset();
    } else {
        stopFlag = false;
        resetFlag = true;
        resetInterval = calculateTriggerInterval();

        // resetIcon.flag = true;
        // resetIcon.mode = true;

        stopIcon.flag = true;
        stopIcon.mode = false;
    }
}

/* PlayData functions --------------------------------------------------------*/

void Controller::startBeatPlayData(uint8_t beatNum_) {
    BeatPlayData &bD = sD.beatPlayData[beatNum_];

    sD.active = true;
    sD.activeBeatNum = beatNum_;

    bD.active = true;
    bD.sdramReadActive = true;
    bD.noteData = sD.startData.noteData;

    system.midi.txTriggerNoteOn[beatNum_] = true;
    system.midi.txDataNoteOn[beatNum_] = bD.noteData;

    /* OscPlayData */

    for (uint8_t i = 0; i < kOscLibrarySize; i++) {
        Osc &osc_ = osc[i];
        OscPlayData &oD = bD.oscPlayData[i];
        ArpegPlayData &aD = bD.arpegPlayData[i];
        if ((osc_.active) && (osc_.wavetableLoaded != kInitialOscWavetable)) {
            if ((key.arpeg) && (key.oscActive[i])) {
                aD.active = true;
                aD.trigger = false;

                aD.freq = kKeyRateDataLibrary[key.rate].rate;

                aD.stepSize = key.stepSize;
                aD.stepCounter = 0;

                aD.chordPointer = key.chordPointer;

                aD.sampleSize = kAudioSampleRate * aD.freq;
                aD.sampleCounter = 0;
                aD.sampleInc = 1;

                (key.order == 4) ? aD.random = true : aD.random = false;
            } else {
                aD.active = false;
            }

            oD.active = true;
            oD.address = kRamWavetableAddressLibrary[i][osc_.playWavetableSector];
            oD.phaseShift = 512 * osc_.phase;
            oD.normalize = osc_.normalize;
            oD.xFlip = osc_.xFlip;
            oD.yFlip = osc_.yFlip;

            oD.normMultiplier = osc_.wavetableSector[osc_.writeWavetableSector].normMultiplier;

            oD.endFlag = false;
            oD.endReadMode = false;
            oD.endLoopMode = false;

            oD.sdramReadActive = true;

            oD.noteBase = bD.noteData + kOscTuneDataLibrary[osc_.tune].tune;

            if (aD.active) {
                uint32_t offset;
                (aD.random) ? offset = rand() % aD.stepSize : offset = 0;
                oD.freqBase = kNoteDataLibrary[oD.noteBase + *(aD.chordPointer + offset)].frequency;
            } else {
                oD.freqBase = kNoteDataLibrary[oD.noteBase].frequency;
            }
            oD.freqCurrent = oD.freqBase;

            oD.levelBase = kFloatDataLibrary[osc_.level / 5].pow2Multiplier;
            oD.levelRange = 1.0;
            oD.levelGround = 0.0;
            oD.levelCurrent = oD.levelBase;
            oD.levelInc = 0.0;

            oD.waveBase = osc_.waveStart;
            oD.waveRange = osc_.waveEnd - osc_.waveStart;
            oD.waveCurrent = oD.waveBase;
            oD.waveInc = 0.0;

            oD.sampleSize = kAudioSampleRate / oD.freqCurrent;
            oD.sampleCounter = 0;
            oD.sampleInc = 1;

            oD.arrayCounter = 0;
            oD.arrayInc = 2048.0f / oD.sampleSize;

            switch (osc_.phase) {
            case 0:
                break;

            case 1:
                oD.sampleCounter += oD.sampleSize / 4;
                oD.arrayCounter += oD.sampleCounter * oD.arrayInc;
                break;

            case 2:
                oD.sampleCounter += (oD.sampleSize / 4) * 2;
                oD.arrayCounter += oD.sampleCounter * oD.arrayInc;
                break;

            case 3:
                oD.sampleCounter += (oD.sampleSize / 4) * 3;
                oD.arrayCounter += oD.sampleCounter * oD.arrayInc;
                break;
            }

            oD.lfoTrigger = false;

            oD.freqLfoActive = false;
            oD.levelLfoActive = false;
            oD.waveLfoActive = false;

            oD.freqLfoNum = -1;
            oD.levelLfoNum = -1;
            oD.waveLfoNum = -1;

            for (uint8_t j = 0; j < kOscLfoLibrarySize; j++) {
                Lfo &lfo_ = osc[i].lfo[j];
                OscLfoPlayData &lD = bD.oscLfoPlayData[i][j];
                if (lfo_.active) {
                    lD.active = lfo_.active;
                    lD.type = lfo_.type;
                    lD.target = lfo_.target;
                    lD.freq = kLfoRateDataLibrary[lfo_.rate].rate;
                    lD.depth = kLfoDepthDataLibrary[lfo_.depth].depth;
                    lD.loop = lfo_.loop;

                    lD.sampleSize = kAudioSampleRate * lD.freq;
                    lD.sampleCounter = 0;
                    lD.sampleInc = 1;

                    lD.arrayCounter = 0;
                    lD.arrayInc = 2048.0f / lD.sampleSize;

                    switch (lfo_.target) {
                    case 0: // wave
                        oD.waveLfoActive = true;
                        oD.waveLfoNum = j;
                        break;

                    case 1: // level
                        oD.levelLfoActive = true;
                        oD.levelLfoNum = j;
                        break;

                    case 2: // freq
                        oD.freqLfoActive = true;
                        oD.freqLfoNum = j;
                        break;
                    }
                } else {
                    lD.active = lfo_.active;
                }
            }

            (oD.levelLfoActive) ? oD.levelSmooth = true : oD.levelSmooth = false;
            (oD.waveLfoActive) ? oD.waveSmooth = true : oD.waveSmooth = false;

            if (oD.freqLfoActive) {
                float data = kLfoDataLibrary[bD.oscLfoPlayData[i][oD.freqLfoNum].type][0];
                oD.freqCurrent = oD.freqBase + (data * bD.oscLfoPlayData[i][oD.freqLfoNum].depth * oD.freqBase);
                oD.sampleSize = kAudioSampleRate / oD.freqCurrent;
                oD.arrayInc = 2048.0f / oD.sampleSize;
            }

            if (oD.levelLfoActive) {
                float data = kLfoDataLibrary[bD.oscLfoPlayData[i][oD.levelLfoNum].type][0];
                oD.levelRange = bD.oscLfoPlayData[i][oD.levelLfoNum].depth;
                oD.levelGround = 1.0 - oD.levelRange;

                oD.levelCurrent = oD.levelGround + (data * bD.oscLfoPlayData[i][oD.levelLfoNum].depth * oD.levelBase);
                oD.levelInc = 0.0;
            }

            if (oD.waveLfoActive) {
                float data = kLfoDataLibrary[bD.oscLfoPlayData[i][oD.waveLfoNum].type][0];
                oD.waveCurrent = oD.waveBase + (data * bD.oscLfoPlayData[i][oD.waveLfoNum].depth * (oD.waveRange));
                oD.waveInc = 0.0;
            }
        } else {
            oD.active = false;
            oD.sdramReadActive = false;
        }
    }

    /* EnvelopePlayData */

    EnvelopePlayData &eD = bD.envelopePlayData;
    eD.active = envelope.active;
    eD.active ? eD.type = envelope.type : eD.type = ENV_OFF;
    eD.active ? eD.curve = envelope.curve : eD.curve = 0x00;
    eD.level = 0.0;
    eD.activeStage = ENV_ATTACK;

    if (kEnvelopeTypeDataLibrary[eD.type].stage[0]) {
        eD.attack.active = true;
        (eD.type == ENV_OFF) ? eD.attack.sampleSize = 2205 : eD.attack.sampleSize = kEnvelopeTimeDataLibrary[envelope.attackTime].sampleSize;
        eD.attack.sampleCounter = 0;
        eD.attack.sampleInc = 1;
        eD.attack.curve = eD.curve;
        eD.attack.dataStart = 0.0;
        eD.attack.dataEnd = 1.0;
        eD.attack.dataCurrent = eD.attack.dataStart;
        eD.attack.dataInc = (float)(eD.attack.dataEnd - eD.attack.dataStart) / eD.attack.sampleSize;
    } else {
        eD.attack.active = false;
        eD.attack.dataCurrent = 0.0;
    }

    if (kEnvelopeTypeDataLibrary[eD.type].stage[1]) {
        eD.decay.active = true;
        eD.decay.sampleSize = kEnvelopeTimeDataLibrary[envelope.decayTime].sampleSize;
        eD.decay.sampleCounter = 0;
        eD.decay.sampleInc = 1;
        eD.decay.curve = eD.curve;
        switch (eD.type) {
        case ENV_OFF:
            break;

        case ENV_ADSR:
            eD.decay.dataStart = 1.0;
            eD.decay.dataEnd = kEnvelopeLevelDataLibrary[envelope.sustainLevel].level;
            break;

        case ENV_ASR:
            break;

        case ENV_AD:
            eD.decay.dataStart = 1.0;
            eD.decay.dataEnd = 0.0;
            break;
        }
        eD.decay.dataInc = (float)(eD.decay.dataEnd - eD.decay.dataStart) / eD.decay.sampleSize;
        eD.decay.dataCurrent = eD.decay.dataStart;
    } else {
        eD.decay.active = false;
        eD.decay.dataCurrent = 0.0;
    }

    if (kEnvelopeTypeDataLibrary[eD.type].stage[2]) {
        eD.sustain.active = true;
        switch (eD.type) {
        case ENV_ADSR:
            eD.sustain.dataCurrent = kEnvelopeLevelDataLibrary[envelope.sustainLevel].level;
            break;

        case ENV_ASR:
        case ENV_OFF:
            eD.sustain.dataCurrent = 1.0;
            break;

        case ENV_AD:
            break;
        }
    } else {
        eD.sustain.active = false;
        eD.sustain.dataCurrent = 0.0;
    }

    if (kEnvelopeTypeDataLibrary[eD.type].stage[3]) {
        eD.release.active = true;
        (eD.type == ENV_OFF)
            ? eD.release.sampleSize = 2205
            : eD.release.sampleSize = kEnvelopeTimeDataLibrary[envelope.releaseTime].sampleSize;
        eD.release.sampleCounter = 0;
        eD.release.sampleInc = 1;
        eD.release.curve = envelope.curve;
        switch (eD.type) {
        case ENV_ADSR:
            eD.release.dataStart = kEnvelopeLevelDataLibrary[envelope.sustainLevel].level;
            eD.release.dataEnd = 0.0;
            break;

        case ENV_ASR:
        case ENV_OFF:
            eD.release.dataStart = 1.0;
            eD.release.dataEnd = 0.0;
            break;

        case ENV_AD:
            eD.release.dataStart = 1.0;
            eD.release.dataEnd = 0.0;
            break;
        }
        eD.release.dataInc = (float)(eD.release.dataEnd - eD.release.dataStart) / eD.release.sampleSize;
        eD.release.dataCurrent = eD.release.dataStart;
    } else {
        eD.release.active = false;
        eD.release.dataCurrent = 0.0;
    }
}

void Controller::releaseBeatPlayData(uint8_t beatNum_) {
    BeatPlayData &bD = sD.beatPlayData[beatNum_];
    EnvelopePlayData &eD = bD.envelopePlayData;

    system.midi.txTriggerNoteOff[beatNum_] = true;
    system.midi.txDataNoteOff[beatNum_] = bD.noteData;

    if ((bD.active) && (eD.activeStage != ENV_END)) {
        switch (eD.activeStage) {
        case ENV_NONE:
            break;

        case ENV_ATTACK:
            switch (eD.type) {
            case ENV_ADSR:
            case ENV_ASR:
            case ENV_OFF:
                eD.activeStage = ENV_RELEASE;
                eD.release.dataStart = eD.level;
                eD.release.dataInc = (float)(eD.release.dataEnd - eD.release.dataStart) / eD.release.sampleSize;
                eD.release.dataCurrent = eD.release.dataStart;
                break;

            case ENV_AD:
                eD.activeStage = ENV_DECAY;
                eD.decay.dataStart = eD.level;
                eD.decay.dataInc = (float)(eD.decay.dataEnd - eD.decay.dataStart) / eD.decay.sampleSize;
                eD.decay.dataCurrent = eD.decay.dataStart;
                break;
            }
            break;

        case ENV_DECAY:
            switch (eD.type) {
            case ENV_ADSR:
                eD.activeStage = ENV_RELEASE;
                eD.release.dataStart = eD.level;
                eD.release.dataInc = (float)(eD.release.dataEnd - eD.release.dataStart) / eD.release.sampleSize;
                eD.release.dataCurrent = eD.release.dataStart;
                break;

            case ENV_ASR:
            case ENV_OFF:
            case ENV_AD:
                break;
            }
            break;

        case ENV_SUSTAIN:
            switch (eD.type) {
            case ENV_ADSR:
            case ENV_ASR:
            case ENV_OFF:
                eD.activeStage = ENV_RELEASE;
                eD.release.dataStart = eD.level;
                eD.release.dataInc = (float)(eD.release.dataEnd - eD.release.dataStart) / eD.release.sampleSize;
                eD.release.dataCurrent = eD.release.dataStart;
                break;

            case ENV_AD:
                break;
            }
            break;

        case ENV_RELEASE:
            switch (eD.type) {
            case ENV_ADSR:
            case ENV_ASR:
            case ENV_OFF:
            case ENV_AD:
                break;
            }
            break;

        case ENV_END:
            break;
        }
    }
}

void Controller::endBeatPlayData(uint8_t beatNum_) {
    BeatPlayData &bD = sD.beatPlayData[beatNum_];
    EnvelopePlayData &eD = bD.envelopePlayData;

    if ((bD.active) && (eD.activeStage != ENV_END)) {
        eD.activeStage = ENV_END;
        eD.end.sampleSize = 2205;
        eD.end.sampleCounter = 0;
        eD.end.sampleInc = 1;
        eD.end.dataStart = eD.level;
        eD.end.dataEnd = 0.0;
        eD.end.dataInc = (float)(eD.end.dataEnd - eD.end.dataStart) / eD.end.sampleSize;
        eD.end.dataCurrent = eD.end.dataStart;

        bD.oscPlayData[0].endFlag = true;
        bD.oscPlayData[1].endFlag = true;
    }
}

void Controller::calculateBeatPlayData(uint8_t beatNum_) {
    BeatPlayData &bD = sD.beatPlayData[beatNum_];
    EnvelopePlayData &eD = bD.envelopePlayData;

    if (bD.active) {
        /* OscPlayData */
        for (uint8_t i = 0; i < kOscLibrarySize; i++) {
            OscPlayData &oD = bD.oscPlayData[i];
            ArpegPlayData &aD = bD.arpegPlayData[i];
            if (oD.active) {
                if (oD.lfoTrigger) {
                    for (uint8_t j = 0; j < kOscLfoLibrarySize; j++) {
                        OscLfoPlayData &lD = bD.oscLfoPlayData[i][j];
                        if (lD.active) {
                            uint16_t arrayCounterLo = (uint16_t)lD.arrayCounter;
                            uint16_t arrayCounterHi = (uint16_t)lD.arrayCounter + 1;

                            if (arrayCounterHi == 2048)
                                arrayCounterHi = 0;

                            float arrayCounterDiff = lD.arrayCounter - arrayCounterLo;

                            float dataLo = kLfoDataLibrary[lD.type][arrayCounterLo];
                            float dataHi = kLfoDataLibrary[lD.type][arrayCounterHi];
                            lD.data = dataLo + ((dataHi - dataLo) * arrayCounterDiff);
                        }
                    }

                    if (aD.trigger) {
                        aD.trigger = false;
                        aD.stepCounter += 1;
                        if (aD.stepCounter == aD.stepSize) {
                            aD.stepCounter = 0;
                        }
                        uint32_t offset;
                        (aD.random) ? offset = rand() % aD.stepSize : offset = aD.stepCounter;
                        oD.freqCurrent = kNoteDataLibrary[oD.noteBase + *(aD.chordPointer + offset)].frequency;
                        oD.sampleSize = kAudioSampleRate / oD.freqCurrent;
                        oD.arrayInc = 2048.0f / oD.sampleSize;
                    }

                    if (oD.freqLfoActive) {
                        if (aD.active) {
                            uint32_t offset;
                            (aD.random) ? offset = rand() % aD.stepSize : offset = aD.stepCounter;
                            float freqTemp = kNoteDataLibrary[oD.noteBase + *(aD.chordPointer + offset)].frequency;
                            oD.freqCurrent = freqTemp + (bD.oscLfoPlayData[i][oD.freqLfoNum].data * bD.oscLfoPlayData[i][oD.freqLfoNum].depth * freqTemp);
                        } else {
                            oD.freqCurrent = oD.freqBase + (bD.oscLfoPlayData[i][oD.freqLfoNum].data * bD.oscLfoPlayData[i][oD.freqLfoNum].depth * oD.freqBase);
                        }
                        oD.sampleSize = kAudioSampleRate / oD.freqCurrent;
                        oD.arrayInc = 2048.0f / oD.sampleSize;
                    }

                    if (oD.levelLfoActive) {
                        if (oD.levelSmooth) {
                            oD.levelInc = (oD.levelGround + (bD.oscLfoPlayData[i][oD.levelLfoNum].data * bD.oscLfoPlayData[i][oD.levelLfoNum].depth * oD.levelBase) - oD.levelCurrent) / oD.sampleSize;
                            oD.levelCurrent += oD.levelInc;
                        } else {
                            oD.levelInc = 0.0;
                            oD.levelCurrent = oD.levelGround + (bD.oscLfoPlayData[i][oD.levelLfoNum].data * bD.oscLfoPlayData[i][oD.levelLfoNum].depth * oD.levelBase);
                        }
                    }

                    if (oD.waveLfoActive) {
                        if (oD.waveSmooth) {
                            float waveRef = oD.waveCurrent - oD.waveBase;
                            oD.waveInc = ((bD.oscLfoPlayData[i][oD.waveLfoNum].data * bD.oscLfoPlayData[i][oD.waveLfoNum].depth * oD.waveRange) - waveRef) / oD.sampleSize;
                            oD.waveCurrent += oD.waveInc;
                        } else {
                            oD.waveInc = 0.0;
                            oD.waveCurrent = oD.waveBase + (bD.oscLfoPlayData[i][oD.waveLfoNum].data * bD.oscLfoPlayData[i][oD.waveLfoNum].depth * oD.waveRange);
                        }
                    }

                    oD.sampleCounter = 0;
                    oD.arrayCounter = 0;

                    oD.lfoTrigger = false;
                }

                if (aD.active) {
                    aD.sampleCounter += 1;
                    if (aD.sampleCounter == aD.sampleSize) {
                        aD.sampleCounter = 0;
                        aD.trigger = true;
                    }
                }

                if (oD.endLoopMode) {
                    oD.outputData = (oD.data[oD.sampleCounter]) * oD.levelCurrent;
                    oD.sampleCounter += oD.sampleInc;
                    if (oD.sampleCounter == oD.sampleSize) {
                        oD.sampleCounter = 0;
                    }
                } else {
                    if ((oD.waveLfoActive) && (oD.waveSmooth)) {
                        uint16_t waveCounterLo = (uint16_t)oD.waveCurrent;
                        uint16_t waveCounterHi = (uint16_t)oD.waveCurrent + 1;
                        float waveCounterDiff = oD.waveCurrent - waveCounterLo;

                        float arrayCounter;
                        (oD.yFlip) ? arrayCounter = 2047 - oD.arrayCounter : arrayCounter = oD.arrayCounter;

                        uint16_t arrayCounterLo = (uint16_t)arrayCounter;
                        uint16_t arrayCounterHi = (uint16_t)arrayCounter + 1;
                        if (arrayCounterHi == 2048)
                            arrayCounterHi = 0;
                        float arrayCounterDiff = arrayCounter - arrayCounterLo;

                        int16_t *dataLoLo = (int16_t *)(oD.address + ((waveCounterLo * 4096) + (arrayCounterLo * 2)));
                        int16_t *dataLoHi = (int16_t *)(oD.address + ((waveCounterLo * 4096) + (arrayCounterHi * 2)));
                        int16_t dataLo = *dataLoLo + ((*dataLoHi - *dataLoLo) * arrayCounterDiff);

                        int16_t *dataHiLo = (int16_t *)(oD.address + ((waveCounterHi * 4096) + (arrayCounterLo * 2)));
                        int16_t *dataHiHi = (int16_t *)(oD.address + ((waveCounterHi * 4096) + (arrayCounterHi * 2)));
                        int16_t dataHi = *dataHiLo + ((*dataHiHi - *dataHiLo) * arrayCounterDiff);

                        oD.data[oD.sampleCounter] = dataLo + ((dataHi - dataLo) * waveCounterDiff);
                        if (oD.normalize)
                            oD.data[oD.sampleCounter] *= oD.normMultiplier;
                        if (oD.xFlip)
                            oD.data[oD.sampleCounter] *= -1;
                        oD.outputData = (oD.data[oD.sampleCounter]) * oD.levelCurrent;
                    } else {
                        float arrayCounter;
                        (oD.yFlip) ? arrayCounter = 2047 - oD.arrayCounter : arrayCounter = oD.arrayCounter;

                        uint16_t arrayCounterLo = (uint16_t)arrayCounter;
                        uint16_t arrayCounterHi = (uint16_t)arrayCounter + 1;
                        if (arrayCounterHi == 2048)
                            arrayCounterHi = 0;
                        float arrayCounterDiff = arrayCounter - arrayCounterLo;

                        uint16_t waveCounter = (uint16_t)oD.waveCurrent;

                        int16_t *dataLo = (int16_t *)(oD.address + ((waveCounter * 4096) + (arrayCounterLo * 2)));
                        int16_t *dataHi = (int16_t *)(oD.address + ((waveCounter * 4096) + (arrayCounterHi * 2)));
                        oD.data[oD.sampleCounter] =
                            *dataLo + ((*dataHi - *dataLo) * arrayCounterDiff);
                        if (oD.normalize)
                            oD.data[oD.sampleCounter] *= oD.normMultiplier;
                        if (oD.xFlip)
                            oD.data[oD.sampleCounter] *= -1;
                        oD.outputData = (oD.data[oD.sampleCounter]) * oD.levelCurrent;
                    }

                    if (oD.levelSmooth)
                        oD.levelCurrent += oD.levelInc;
                    if ((oD.waveSmooth) && (!oD.endReadMode) && (!oD.endLoopMode))
                        oD.waveCurrent += oD.waveInc;

                    oD.sampleCounter += oD.sampleInc;
                    if (oD.sampleCounter == oD.sampleSize) {
                        oD.sampleCounter = 0;
                        oD.arrayCounter = 0;
                        if (oD.endFlag) {
                            oD.endFlag = false;
                            oD.endReadMode = true;
                            oD.endLoopMode = false;
                        } else if (oD.endReadMode) {
                            oD.endReadMode = false;
                            oD.endLoopMode = true;
                            oD.sdramReadActive = false;
                            if ((!bD.oscPlayData[0].sdramReadActive) && (!bD.oscPlayData[1].sdramReadActive)) {
                                bD.sdramReadActive = false;
                            }
                        }

                        if ((!oD.endFlag) && (!oD.endReadMode) && (!oD.endLoopMode))
                            oD.lfoTrigger = true;
                    } else {
                        oD.arrayCounter += oD.arrayInc;
                    }

                    for (uint8_t j = 0; j < kOscLfoLibrarySize; j++) {
                        OscLfoPlayData &lD = bD.oscLfoPlayData[i][j];
                        if (lD.active) {
                            lD.sampleCounter += 1;
                            if (lD.sampleCounter == lD.sampleSize) {
                                lD.sampleCounter = 0;
                                lD.arrayCounter = 0;
                                if (!lD.loop)
                                    lD.active = false;
                            } else {
                                lD.arrayCounter += lD.arrayInc;
                            }
                        }
                    }
                }
            } else {
                oD.outputData = 0;
            }
        }

        /* EnvelopePlayData */

        eD.inputData = ((bD.oscPlayData[0].outputData + bD.oscPlayData[1].outputData) << 8);

        switch (eD.activeStage) {
        case ENV_NONE:
            eD.level = 0.0;
            break;

        case ENV_ATTACK:
            eD.attack.sampleCounter += eD.attack.sampleInc = 1;
            eD.attack.dataCurrent += eD.attack.dataInc;
            eD.level = eD.attack.dataCurrent;
            if (eD.attack.sampleCounter == eD.attack.sampleSize) {
                switch (eD.type) {
                case ENV_AD:
                    eD.activeStage = ENV_DECAY;
                    break;

                case ENV_ASR:
                case ENV_OFF:
                    eD.activeStage = ENV_SUSTAIN;
                    break;

                case ENV_ADSR:
                    eD.activeStage = ENV_DECAY;
                    break;
                }
            }
            break;

        case ENV_DECAY:
            eD.decay.sampleCounter += eD.decay.sampleInc = 1;
            eD.decay.dataCurrent += eD.decay.dataInc;
            eD.level = eD.decay.dataCurrent;
            if (eD.decay.sampleCounter == eD.decay.sampleSize) {
                switch (eD.type) {
                case ENV_AD:
                    bD.active = false;
                    bD.sdramReadActive = false;
                    eD.activeStage = ENV_OFF;
                    bD.oscPlayData[0].active = false;
                    bD.oscPlayData[1].active = false;
                    sD.active = false;
                    break;

                case ENV_ASR:
                case ENV_OFF:
                    break;

                case ENV_ADSR:
                    eD.activeStage = ENV_SUSTAIN;
                    break;
                }
            }
            break;

        case ENV_SUSTAIN:
            eD.level = eD.sustain.dataCurrent;
            switch (eD.type) {
            case ENV_AD:
                break;

            case ENV_ASR:
            case ENV_OFF:
                break;

            case ENV_ADSR:
                break;
            }
            break;

        case ENV_RELEASE:
            eD.release.sampleCounter += eD.release.sampleInc;
            eD.release.dataCurrent += eD.release.dataInc;
            eD.level = eD.release.dataCurrent;
            if (eD.release.sampleCounter == eD.release.sampleSize) {
                bD.active = false;
                bD.sdramReadActive = false;
                eD.activeStage = ENV_OFF;
                bD.oscPlayData[0].active = false;
                bD.oscPlayData[1].active = false;
                sD.active = false;
            }
            break;

        case ENV_END:
            eD.end.sampleCounter += eD.end.sampleInc;
            eD.end.dataCurrent += eD.end.dataInc;
            eD.level = eD.end.dataCurrent;
            if (eD.end.sampleCounter == eD.end.sampleSize) {
                bD.active = false;
                bD.sdramReadActive = false;
                eD.activeStage = ENV_OFF;
                bD.oscPlayData[0].active = false;
                bD.oscPlayData[1].active = false;
                sD.active = false;
            }
            break;
        }

        switch (eD.curve) {
        case 0: // log
            eD.outputData = eD.inputData * eD.level;
            break;

        case 1: // lin
            eD.outputData = eD.inputData * pow(eD.level, 2);
            break;

        case 2: // exp
            eD.outputData = eD.inputData * pow(eD.level, 3);
            break;

        default:
            eD.outputData = eD.inputData * eD.level;
            break;
        }

        bD.data = eD.outputData;
    } else {
        bD.data = 0;
    }
}

bool Controller::checkSdramReadActive() {
    if ((!sD.beatPlayData[0].sdramReadActive) && (!sD.beatPlayData[1].sdramReadActive)) {
        return false;
    } else {
        return true;
    }
}

/* Interrupt functions -------------------------------------------------------*/

void Controller::interruptPlay() {
    // metro pre count state
    if (metronome.precountState) {
        // trigger bar beat
        if ((metronome.precounter % barInterval == 0) && (!system.sync.slaveMode)) {
            mD.active = true;
            mD.counter = 0;
            mD.counterMax = kMetroSize - 1;
            mD.ramAddress = kRamMetronomeAddressLibrary[metronome.sample][0];
            mD.volumeMultiplier = 0.75f * metronome.volumeFloat;

            metronome.countDownFlag = true;
        }
        // trigger measure beat
        else if ((metronome.precounter % measureInterval == 0) && (!system.sync.slaveMode)) {
            mD.active = true;
            mD.counter = 0;
            mD.counterMax = kMetroSize - 1;
            mD.ramAddress = kRamMetronomeAddressLibrary[metronome.sample][1];
            mD.volumeMultiplier = 0.75f * metronome.volumeFloat;

            metronome.countDownFlag = true;
        }
        // increment metro count play interval
        if (metronome.precounter < metronome.precounterMax) {
            metronome.precounter += 1;
        } else {
            metronome.precounter = 0;
            metronome.precountState = false;
        }
    }

    // play & record state
    else {
        // check metronome
        if ((recordActive) && (metronome.active) && (playInterval != songInterval)) {
            // trigger bar beat
            if ((playInterval % barInterval == 0) && (!system.sync.slaveMode)) {
                mD.active = true;
                mD.counter = 0;
                mD.counterMax = kMetroSize - 1;
                mD.ramAddress = kRamMetronomeAddressLibrary[metronome.sample][0];
                mD.volumeMultiplier = 0.75f * metronome.volumeFloat;
            }
            // trigger measure beat
            else if ((playInterval % measureInterval == 0) && (!system.sync.slaveMode)) {
                mD.active = true;
                mD.counter = 0;
                mD.counterMax = kMetroSize - 1;
                mD.ramAddress = kRamMetronomeAddressLibrary[metronome.sample][1];
                mD.volumeMultiplier = 0.75f * metronome.volumeFloat;
            }
        }

        // beat sync
        if ((system.syncOut == 2) && (playInterval % kMeasureHalfInterval == 0)) {
            SYNC_OUT_ON;
            startBeatSyncTimer();
        }

        // check beat
        Bank &bank = song.bankLibrary[activeBankNum];
        if (bank.lastActiveBeatNum != -1) {
            if (bank.beatLibrary[bank.nextBeatNum].startInterval == playInterval) {
                sD.startData.noteData = (bank.beatLibrary[bank.nextBeatNum].octave * 12) + (key.note + bank.beatLibrary[bank.nextBeatNum].note - 1);

                // play note
                if ((!recordActive) || ((recordActive) && (recordBeatNum == -1))) {
                    switch (sD.activeBeatNum) {
                    case -1:
                        sD.startData.active = true;
                        sD.startData.beatNum = 0;
                        break;

                    case 0:
                        sD.startData.active = true;
                        sD.startData.beatNum = 1;

                        sD.endData.active = true;
                        sD.endData.beatNum = 0;
                        break;

                    case 1:
                        sD.startData.active = true;
                        sD.startData.beatNum = 0;

                        sD.endData.active = true;
                        sD.endData.beatNum = 1;
                        break;
                    }
                }

                bank.playBeatNum = bank.nextBeatNum;
                (bank.nextBeatNum < bank.lastActiveBeatNum) ? bank.nextBeatNum += 1 : bank.nextBeatNum = 0;
            } else if ((bank.beatLibrary[bank.playBeatNum].endInterval == playInterval)) {
                if ((!recordActive) || ((recordActive) && (bank.playBeatNum != bank.playBeatNum))) {
                    sD.releaseData.active = true;
                    sD.releaseData.beatNum = sD.activeBeatNum;
                }
            }
        }

        // increment play interval
        playInterval += 1;
        if (playInterval < songInterval) {
            if (stopFlag) {
                if (stopInterval == playInterval) {
                    stop();
                    if (playInterval == (songInterval - 1)) {
                        playInterval = 0;
                        if (bankShiftFlag) {
                            bankShiftFlag = false;
                            activeBankNum = targetBankNum;
                            bankActionFlag = true;
                        }
                        Bank &bank = song.bankLibrary[activeBankNum];
                        if (bank.lastActiveBeatNum != -1) {
                            bank.nextBeatNum = 0;
                        } else {
                            bank.nextBeatNum = -1;
                        }

                        lcd_restartPlay();
                    }
                } else if (playInterval % (measureInterval / 2) == 0) {
                    stopIcon.flag = true;
                    stopIcon.mode = !stopIcon.mode;
                }
            }

            if (resetFlag) {
                if (resetInterval == playInterval) {
                    reset();
                } else if (playInterval % (measureInterval / 2) == 0) {
                    resetIcon.flag = true;
                    resetIcon.mode = !resetIcon.mode;
                }
            }
        } else {
            if (recordBeatNum != -1)
                song_endRecordBeat(activeBankNum, playInterval);
            playInterval = 0;
            if (bankShiftFlag) {
                bankShiftFlag = false;
                activeBankNum = targetBankNum;
                bankActionFlag = true;
            }
            Bank &bank = song.bankLibrary[activeBankNum];
            if (bank.lastActiveBeatNum != -1) {
                bank.nextBeatNum = 0;
            } else {
                bank.nextBeatNum = -1;
            }

            lcd_restartPlay();
        }
    }
}

void Controller::interruptAudioMetronome() {
    audioMetronome = 0;

    if (mD.active) {
        uint32_t address = (uint32_t)(mD.ramAddress + (mD.counter * 3));
        audioMetronome = (sdram_read24BitAudio(address) << 8) * mD.volumeMultiplier;

        if (mD.counter < mD.counterMax) {
            mD.counter += 1;
        } else {
            mD.counter = 0;
            mD.active = false;
        }
    }
}

void Controller::interruptAudioSong() {
    audioSong = 0;

    // start data
    if ((sD.startData.active) && (checkSdramReadActive() == false)) {
        startBeatPlayData(sD.startData.beatNum);
        sD.startData.active = false;
    }
    // release data
    else if (sD.releaseData.active) {
        releaseBeatPlayData(sD.releaseData.beatNum);
        sD.releaseData.active = false;
    }
    // end data
    else if (sD.endData.active) {
        endBeatPlayData(sD.endData.beatNum);
        sD.endData.active = false;
    }
    // calculate play data
    for (uint8_t i = 0; i < 2; i++) {
        calculateBeatPlayData(i);
        audioSong += sD.beatPlayData[i].data;
    }

    audioSong *= system.volumeFloat;

    /*
        if (wavRead) {
            wavData[wavCounter] = sD.beatPlayData[0].data;
            wavCounter += 1;
            if (wavCounter >= 88200) {
                wavRead = false;
                wavWriteFlag = true;
            }
        }
    */
}

void Controller::interruptAudioLpf() {
    int32_t input = audioSong;

    if (lpf.active) {
        lpf.dataIn[0] = input;
        lpf.dataOut[0] = (lpf.a0 * lpf.dataIn[0]) + (lpf.a1 * lpf.dataIn[1]) + (lpf.a2 * lpf.dataIn[2]) - (lpf.b1 * lpf.dataOut[1]) - (lpf.b2 * lpf.dataOut[2]);

        lpf.dataIn[2] = lpf.dataIn[1];
        lpf.dataIn[1] = lpf.dataIn[0];

        lpf.dataOut[2] = lpf.dataOut[1];
        lpf.dataOut[1] = lpf.dataOut[0];

        audioLpf = (lpf.dataOut[0] * lpf.wet) + (input * lpf.dry);
    } else {
        audioLpf = input;
    }
}

void Controller::interruptAudioEq() {
    int32_t input = audioLpf;
    int32_t output;

    if ((eq.active) || (eq.genTransition.active)) {
        // low shelf filter
        eq.filterLowShelf.dataIn[0] = input;
        if (eq.gainLowShelf != kEqGainZero) {
            eq.filterLowShelf.dataOut[0] = (eq.filterLowShelf.a0 * eq.filterLowShelf.dataIn[0]) + (eq.filterLowShelf.a1 * eq.filterLowShelf.dataIn[1]) +
                                           (eq.filterLowShelf.a2 * eq.filterLowShelf.dataIn[2]) - (eq.filterLowShelf.b1 * eq.filterLowShelf.dataOut[1]) - (eq.filterLowShelf.b2 * eq.filterLowShelf.dataOut[2]);
        } else {
            eq.filterLowShelf.dataOut[0] = eq.filterLowShelf.dataIn[0];
        }
        eq.filterLowShelf.dataIn[2] = eq.filterLowShelf.dataIn[1];
        eq.filterLowShelf.dataIn[1] = eq.filterLowShelf.dataIn[0];

        eq.filterLowShelf.dataOut[2] = eq.filterLowShelf.dataOut[1];
        eq.filterLowShelf.dataOut[1] = eq.filterLowShelf.dataOut[0];

        // high shelf filter
        eq.filterHighShelf.dataIn[0] = eq.filterLowShelf.dataOut[0];
        if (eq.gainHighShelf != kEqGainZero) {
            eq.filterHighShelf.dataOut[0] = (eq.filterHighShelf.a0 * eq.filterHighShelf.dataIn[0]) + (eq.filterHighShelf.a1 * eq.filterHighShelf.dataIn[1]) +
                                            (eq.filterHighShelf.a2 * eq.filterHighShelf.dataIn[2]) - (eq.filterHighShelf.b1 * eq.filterHighShelf.dataOut[1]) - (eq.filterHighShelf.b2 * eq.filterHighShelf.dataOut[2]);
        } else {
            eq.filterHighShelf.dataOut[0] = eq.filterHighShelf.dataIn[0];
        }
        eq.filterHighShelf.dataIn[2] = eq.filterHighShelf.dataIn[1];
        eq.filterHighShelf.dataIn[1] = eq.filterHighShelf.dataIn[0];

        eq.filterHighShelf.dataOut[2] = eq.filterHighShelf.dataOut[1];
        eq.filterHighShelf.dataOut[1] = eq.filterHighShelf.dataOut[0];

        // peak 1
        eq.filterPeak[0].dataIn[0] = eq.filterHighShelf.dataOut[0];
        if (eq.gainPeak[0] != kEqGainZero) {
            eq.filterPeak[0].dataOut[0] = (eq.filterPeak[0].a0 * eq.filterPeak[0].dataIn[0]) + (eq.filterPeak[0].a1 * eq.filterPeak[0].dataIn[1]) +
                                          (eq.filterPeak[0].a2 * eq.filterPeak[0].dataIn[2]) - (eq.filterPeak[0].b1 * eq.filterPeak[0].dataOut[1]) - (eq.filterPeak[0].b2 * eq.filterPeak[0].dataOut[2]);
        } else {
            eq.filterPeak[0].dataOut[0] = eq.filterPeak[0].dataIn[0];
        }
        eq.filterPeak[0].dataIn[2] = eq.filterPeak[0].dataIn[1];
        eq.filterPeak[0].dataIn[1] = eq.filterPeak[0].dataIn[0];

        eq.filterPeak[0].dataOut[2] = eq.filterPeak[0].dataOut[1];
        eq.filterPeak[0].dataOut[1] = eq.filterPeak[0].dataOut[0];

        // peak 2
        eq.filterPeak[1].dataIn[0] = eq.filterPeak[0].dataOut[0];
        if (eq.gainPeak[1] != kEqGainZero) {
            eq.filterPeak[1].dataOut[0] = (eq.filterPeak[1].a0 * eq.filterPeak[1].dataIn[0]) + (eq.filterPeak[1].a1 * eq.filterPeak[1].dataIn[1]) +
                                          (eq.filterPeak[1].a2 * eq.filterPeak[1].dataIn[2]) - (eq.filterPeak[1].b1 * eq.filterPeak[1].dataOut[1]) - (eq.filterPeak[1].b2 * eq.filterPeak[1].dataOut[2]);
        } else {
            eq.filterPeak[1].dataOut[0] = eq.filterPeak[1].dataIn[0];
        }
        eq.filterPeak[1].dataIn[2] = eq.filterPeak[1].dataIn[1];
        eq.filterPeak[1].dataIn[1] = eq.filterPeak[1].dataIn[0];

        eq.filterPeak[1].dataOut[2] = eq.filterPeak[1].dataOut[1];
        eq.filterPeak[1].dataOut[1] = eq.filterPeak[1].dataOut[0];

        // peak 3
        eq.filterPeak[2].dataIn[0] = eq.filterPeak[1].dataOut[0];
        if (eq.gainPeak[2] != kEqGainZero) {
            eq.filterPeak[2].dataOut[0] = (eq.filterPeak[2].a0 * eq.filterPeak[2].dataIn[0]) + (eq.filterPeak[2].a1 * eq.filterPeak[2].dataIn[1]) +
                                          (eq.filterPeak[2].a2 * eq.filterPeak[2].dataIn[2]) - (eq.filterPeak[2].b1 * eq.filterPeak[2].dataOut[1]) - (eq.filterPeak[2].b2 * eq.filterPeak[2].dataOut[2]);
        } else {
            eq.filterPeak[2].dataOut[0] = eq.filterPeak[2].dataIn[0];
        }
        eq.filterPeak[2].dataIn[2] = eq.filterPeak[2].dataIn[1];
        eq.filterPeak[2].dataIn[1] = eq.filterPeak[2].dataIn[0];

        eq.filterPeak[2].dataOut[2] = eq.filterPeak[2].dataOut[1];
        eq.filterPeak[2].dataOut[1] = eq.filterPeak[2].dataOut[0];

        // peak 4
        eq.filterPeak[3].dataIn[0] = eq.filterPeak[2].dataOut[0];
        if (eq.gainPeak[3] != kEqGainZero) {
            eq.filterPeak[3].dataOut[0] = (eq.filterPeak[3].a0 * eq.filterPeak[3].dataIn[0]) + (eq.filterPeak[3].a1 * eq.filterPeak[3].dataIn[1]) +
                                          (eq.filterPeak[3].a2 * eq.filterPeak[3].dataIn[2]) - (eq.filterPeak[3].b1 * eq.filterPeak[3].dataOut[1]) - (eq.filterPeak[3].b2 * eq.filterPeak[3].dataOut[2]);
        } else {
            eq.filterPeak[3].dataOut[0] = eq.filterPeak[3].dataIn[0];
        }
        eq.filterPeak[3].dataIn[2] = eq.filterPeak[3].dataIn[1];
        eq.filterPeak[3].dataIn[1] = eq.filterPeak[3].dataIn[0];

        eq.filterPeak[3].dataOut[2] = eq.filterPeak[3].dataOut[1];
        eq.filterPeak[3].dataOut[1] = eq.filterPeak[3].dataOut[0];

        output = eq.filterPeak[3].dataOut[0];
    } else {
        output = input;
    }

    if (eq.genTransition.active) {
        audioEq = (eq.genTransition.activeWet * output) + (eq.genTransition.activeDry * input);
    } else {
        audioEq = output;
    }
}

void Controller::interruptAudioFilter() {
    int32_t outputA = processAudioFilter(0, audioEq);
    int32_t outputB = processAudioFilter(1, outputA);
    audioFilter = outputB;
}

void Controller::interruptAudioEffect() {
    int32_t outputA = processAudioEffect(0, audioFilter);
    int32_t outputB = processAudioEffect(1, outputA);
    audioEffect = outputB;

    if ((audioEffect >= INT24_MAX) || (audioEffect <= INT24_MIN)) {
        limitAlertShowFlag = true;
        if ((system.limiter) && (!system.volumeTransition.active) && (system.volume > kMinSystemVolume)) {
            system_setVolume(system.volume - 1);
        }
    }
}

void Controller::interruptAudioReverb() {
    int32_t input = (audioEffect << 8);
    int32_t output_L;
    int32_t output_R;

    if ((reverb.active) || (reverb.genTransition.active)) {
        // preDelay

        reverb.preDelayBuffer[reverb.preDelay_recordInterval] = input;
        int32_t reverbInput = reverb.preDelayBuffer[reverb.preDelay_playInterval];

        if (++reverb.preDelay_playInterval > (reverb.kPreDelayBufferSize - 1))
            reverb.preDelay_playInterval = 0;
        if (++reverb.preDelay_recordInterval > (reverb.kPreDelayBufferSize - 1))
            reverb.preDelay_recordInterval = 0;

        // comb filter

        int32_t inputData = 0;
        int32_t combOutput = 0;

        inputData = reverbInput * 0.015;

        reverb.comb1Out = reverb.comb1Buffer[reverb.comb1Index];
        reverb.comb1Filter = (reverb.comb1Out * reverb.combDecay2) + (reverb.comb1Filter * reverb.combDecay1);
        reverb.comb1Buffer[reverb.comb1Index] = inputData + (reverb.comb1Filter * reverb.combFeedback);
        if (++reverb.comb1Index >= reverb.comb1Size)
            reverb.comb1Index = 0;
        combOutput += reverb.comb1Out;

        reverb.comb2Out = reverb.comb2Buffer[reverb.comb2Index];
        reverb.comb2Filter = (reverb.comb2Out * reverb.combDecay2) + (reverb.comb2Filter * reverb.combDecay1);
        reverb.comb2Buffer[reverb.comb2Index] = inputData + (reverb.comb2Filter * reverb.combFeedback);
        if (++reverb.comb2Index >= reverb.comb2Size)
            reverb.comb2Index = 0;
        combOutput += reverb.comb2Out;

        reverb.comb3Out = reverb.comb3Buffer[reverb.comb3Index];
        reverb.comb3Filter = (reverb.comb3Out * reverb.combDecay2) + (reverb.comb3Filter * reverb.combDecay1);
        reverb.comb3Buffer[reverb.comb3Index] = inputData + (reverb.comb3Filter * reverb.combFeedback);
        if (++reverb.comb3Index >= reverb.comb3Size)
            reverb.comb3Index = 0;
        combOutput += reverb.comb3Out;

        reverb.comb4Out = reverb.comb4Buffer[reverb.comb4Index];
        reverb.comb4Filter = (reverb.comb4Out * reverb.combDecay2) + (reverb.comb4Filter * reverb.combDecay1);
        reverb.comb4Buffer[reverb.comb4Index] = inputData + (reverb.comb4Filter * reverb.combFeedback);
        if (++reverb.comb4Index >= reverb.comb4Size)
            reverb.comb4Index = 0;
        combOutput += reverb.comb4Out;

        reverb.comb5Out = reverb.comb5Buffer[reverb.comb5Index];
        reverb.comb5Filter = (reverb.comb5Out * reverb.combDecay2) + (reverb.comb5Filter * reverb.combDecay1);
        reverb.comb5Buffer[reverb.comb5Index] = inputData + (reverb.comb5Filter * reverb.combFeedback);
        if (++reverb.comb5Index >= reverb.comb5Size)
            reverb.comb5Index = 0;
        combOutput += reverb.comb5Out;

        reverb.comb6Out = reverb.comb6Buffer[reverb.comb6Index];
        reverb.comb6Filter = (reverb.comb6Out * reverb.combDecay2) + (reverb.comb6Filter * reverb.combDecay1);
        reverb.comb6Buffer[reverb.comb6Index] = inputData + (reverb.comb6Filter * reverb.combFeedback);
        if (++reverb.comb6Index >= reverb.comb6Size)
            reverb.comb6Index = 0;
        combOutput += reverb.comb6Out;

        reverb.comb7Out = reverb.comb7Buffer[reverb.comb7Index];
        reverb.comb7Filter = (reverb.comb7Out * reverb.combDecay2) + (reverb.comb7Filter * reverb.combDecay1);
        reverb.comb7Buffer[reverb.comb7Index] = inputData + (reverb.comb7Filter * reverb.combFeedback);
        if (++reverb.comb7Index >= reverb.comb7Size)
            reverb.comb7Index = 0;
        combOutput += reverb.comb7Out;

        reverb.comb8Out = reverb.comb8Buffer[reverb.comb8Index];
        reverb.comb8Filter = (reverb.comb8Out * reverb.combDecay2) + (reverb.comb8Filter * reverb.combDecay1);
        reverb.comb8Buffer[reverb.comb8Index] = inputData + (reverb.comb8Filter * reverb.combFeedback);
        if (++reverb.comb8Index >= reverb.comb8Size)
            reverb.comb8Index = 0;
        combOutput += reverb.comb8Out;

        // allpass filter

        reverb.apass1Out = reverb.apass1Buffer[reverb.apass1Index] - combOutput;
        reverb.apass1Buffer[reverb.apass1Index] = combOutput + (reverb.apass1Buffer[reverb.apass1Index] * reverb.apassFeedback);
        if (++reverb.apass1Index >= reverb.apass1Size)
            reverb.apass1Index = 0;

        reverb.apass2Out =
            reverb.apass2Buffer[reverb.apass2Index] - reverb.apass1Out;
        reverb.apass2Buffer[reverb.apass2Index] = reverb.apass1Out + (reverb.apass2Buffer[reverb.apass2Index] * reverb.apassFeedback);
        if (++reverb.apass2Index >= reverb.apass2Size)
            reverb.apass2Index = 0;

        reverb.apass3Out =
            reverb.apass3Buffer[reverb.apass3Index] - reverb.apass2Out;
        reverb.apass3Buffer[reverb.apass3Index] = reverb.apass2Out + (reverb.apass3Buffer[reverb.apass3Index] * reverb.apassFeedback);
        if (++reverb.apass3Index >= reverb.apass3Size)
            reverb.apass3Index = 0;

        reverb.apass4Out =
            reverb.apass4Buffer[reverb.apass4Index] - reverb.apass3Out;
        reverb.apass4Buffer[reverb.apass4Index] = reverb.apass3Out + (reverb.apass4Buffer[reverb.apass4Index] * reverb.apassFeedback);
        if (++reverb.apass4Index >= reverb.apass4Size)
            reverb.apass4Index = 0;

        int32_t data =
            (reverb.apass4Out * reverb.wetFloat) + (input * reverb.dryFloat);

        int32_t audioReverb;

        if (reverb.genTransition.active) {
            audioReverb = (data * reverb.genTransition.activeWet) + (input * reverb.genTransition.activeDry);
        } else {
            audioReverb = data;
        }

        // surround

        if ((reverb.genTransition.active) && ((reverb.genTransition.mode == REV_MODE_ACTIVE) || (reverb.genTransition.mode == REV_MODE_SURROUND))) {
            reverb.surroundBuffer[reverb.surround_recordInterval] = audioReverb * reverb.genTransition.activeWet;
            output_L = audioReverb;
            output_R = (reverb.surroundBuffer[reverb.surround_playInterval] * reverb.genTransition.activeWet) + (audioReverb * reverb.genTransition.activeDry);
        } else {
            reverb.surroundBuffer[reverb.surround_recordInterval] = audioReverb;
            output_L = audioReverb;
            output_R = reverb.surroundBuffer[reverb.surround_playInterval];
        }

        if (++reverb.surround_playInterval > (reverb.kSurroundBufferSize - 1))
            reverb.surround_playInterval = 0;
        if (++reverb.surround_recordInterval > (reverb.kSurroundBufferSize - 1))
            reverb.surround_recordInterval = 0;
    } else {
        // surround
        output_L = input;
        output_R = input;
    }

    audioReverb_L = output_L;
    audioReverb_R = output_R;
}

void Controller::interruptAudioSend() {
    audioSend_L = (audioReverb_L + audioMetronome) * system.volumeLeftFloat;
    audioSend_R = (audioReverb_R + audioMetronome) * system.volumeRightFloat;

    uint16_t audio_L0 = (uint16_t)((audioSend_L >> 16) & (0xFFFF));
    uint16_t audio_L1 = (uint16_t)(audioSend_L & 0xFFFF);

    uint16_t audio_R0 = (uint16_t)((audioSend_R >> 16) & (0xFFFF));
    uint16_t audio_R1 = (uint16_t)(audioSend_R & 0xFFFF);

    dac.i2s_data[0] = audio_L0;
    dac.i2s_data[1] = audio_L1;

    dac.i2s_data[2] = audio_R0;
    dac.i2s_data[3] = audio_R1;
}

int32_t Controller::processAudioFilter(uint8_t filterNum_, int32_t audio_) {
    Filter &filter_ = filter[filterNum_];
    int32_t input = audio_;
    int32_t output;

    if ((filter_.active) || (filter_.genTransition.active)) {
        filter_.dataIn[0] = input;
        filter_.dataOut[0] = (filter_.a0 * filter_.dataIn[0]) + (filter_.a1 * filter_.dataIn[1]) + (filter_.a2 * filter_.dataIn[2]) - (filter_.b1 * filter_.dataOut[1]) - (filter_.b2 * filter_.dataOut[2]);

        filter_.dataIn[2] = filter_.dataIn[1];
        filter_.dataIn[1] = filter_.dataIn[0];

        filter_.dataOut[2] = filter_.dataOut[1];
        filter_.dataOut[1] = filter_.dataOut[0];

        int32_t data = (filter_.dataOut[0] * filter_.wetFloat) + (input * filter_.dryFloat);

        if (filter_.slope == 0)
            data = (data / 2) + (input / 2);

        if (filter_.genTransition.active) {
            output = (data * filter_.genTransition.activeWet) + (input * filter_.genTransition.activeDry);
        } else {
            output = data;
        }
    } else {
        output = input;
    }

    return output;
}

int32_t Controller::processAudioEffect(uint8_t effectNum_, int32_t audio_) {
    Effect &effect_ = effect[effectNum_];
    int32_t input = audio_;
    int32_t output;

    if ((effect_.active) || (effect_.genTransition.active)) {
        // type selection
        uint8_t type;
        (effect_.genTransition.active) ? type = effect_.genTransition.activeType : type = effect_.type;

        // delay

        if (type == EF_DELAY) {
            Delay &delay_ = effect_.delay;

            volatile int32_t *playPtr = (volatile int32_t *)(effect_.delayAddress + (delay_.playInterval * 4));
            volatile int32_t *recordPtr = (volatile int32_t *)(effect_.delayAddress + (delay_.recordInterval * 4));

            int32_t playData = input + *playPtr;

            *recordPtr = (input * delay_.level) + (*playPtr * delay_.feedback);

            int32_t data = (playData * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }

            if (++delay_.playInterval > (kDelaySize - 1))
                delay_.playInterval = 0;
            if (++delay_.recordInterval > (kDelaySize - 1))
                delay_.recordInterval = 0;
        }

        // chorus

        else if (type == EF_CHORUS) {
            Chorus &chorus_ = effect_.chorus;

            int32_t dataChorus;
            int32_t dataChorusDelay[2];

            for (uint8_t i = 0; i < 2; i++) {
                ChorusDelay &cD = chorus_.chorusDelay[i];

                cD.chorusInterval = cD.playInterval + cD.shiftInterval;

                if (cD.chorusInterval < 0) {
                    cD.chorusInterval += kChorusBufferSize;
                } else if (cD.chorusInterval > kChorusBufferSize) {
                    cD.chorusInterval -= kChorusBufferSize;
                }

                uint16_t interval_Int0 = (uint16_t)cD.chorusInterval;
                uint16_t interval_Int1 = interval_Int0 + 1;
                float remainder = cD.chorusInterval - interval_Int0;

                if (interval_Int1 == kChorusBufferSize)
                    interval_Int1 = 0;

                volatile int32_t *dataPtr0 = (volatile int32_t *)(effect_.chorusAddress + (interval_Int0 * 4));
                volatile int32_t *dataPtr1 = (volatile int32_t *)(effect_.chorusAddress + (interval_Int1 * 4));
                dataChorusDelay[i] = *dataPtr0 + ((*dataPtr1 - *dataPtr0) * remainder);

                cD.shiftInterval += cD.shiftInc;

                if (cD.shiftInterval <= cD.shiftMin) {
                    cD.shiftInc *= -1;
                    cD.shiftInterval = cD.shiftMin;
                }

                if (cD.shiftInterval >= cD.shiftMax) {
                    cD.shiftInc *= -1;
                    cD.shiftInterval = cD.shiftMax;
                }
            }

            dataChorus = (dataChorusDelay[0] * chorus_.chorusDelay[0].mix) + (dataChorusDelay[1] * chorus_.chorusDelay[1].mix);

            volatile int32_t *recordPtr = (volatile int32_t *)(effect_.chorusAddress + (chorus_.recordInterval * 4));

            if (effect_.genTransition.active) {
                *recordPtr = (input + (dataChorus * chorus_.feedback)) * effect_.genTransition.activeRecordWet;
            } else {
                *recordPtr = input + (dataChorus * chorus_.feedback);
            }

            int32_t data = (dataChorus * effect_.wetFloat) + (*recordPtr * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }

            if ((++chorus_.recordInterval) > (kChorusBufferSize - 1))
                chorus_.recordInterval = 0;
            if ((++chorus_.chorusDelay[0].playInterval) > (kChorusBufferSize - 1))
                chorus_.chorusDelay[0].playInterval = 0;
            if ((++chorus_.chorusDelay[1].playInterval) > (kChorusBufferSize - 1))
                chorus_.chorusDelay[1].playInterval = 0;
        }

        // flanger

        else if (type == EF_FLANGER) {
            Flanger &flanger_ = effect_.flanger;

            int32_t dataFlanger;

            flanger_.flangerInterval = flanger_.playInterval + flanger_.shiftInterval;

            if (flanger_.flangerInterval < 0) {
                flanger_.flangerInterval += kFlangerBufferSize;
            } else if (flanger_.flangerInterval > kFlangerBufferSize) {
                flanger_.flangerInterval -= kFlangerBufferSize;
            }

            uint16_t interval_Int0 = (uint16_t)flanger_.flangerInterval;
            uint16_t interval_Int1 = interval_Int0 + 1;
            float remainder = flanger_.flangerInterval - interval_Int0;

            if (interval_Int1 == kFlangerBufferSize)
                interval_Int1 = 0;

            int32_t data0 = effect_.flangerBuffer[interval_Int0];
            int32_t data1 = effect_.flangerBuffer[interval_Int1];
            dataFlanger = data0 + ((data1 - data0) * remainder);

            if (effect_.genTransition.active) {
                effect_.flangerBuffer[flanger_.recordInterval] = (input + (dataFlanger * flanger_.feedback)) * effect_.genTransition.activeRecordWet;
            } else {
                effect_.flangerBuffer[flanger_.recordInterval] = input + (dataFlanger * flanger_.feedback);
            }

            int32_t data = (dataFlanger * effect_.wetFloat) + (effect_.flangerBuffer[flanger_.recordInterval] * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }

            flanger_.shiftInterval += flanger_.shiftInc;

            if (flanger_.shiftInterval <= flanger_.shiftMin) {
                flanger_.shiftInc *= -1;
                flanger_.shiftInterval = flanger_.shiftMin;
            }

            if (flanger_.shiftInterval >= flanger_.shiftMax) {
                flanger_.shiftInc *= -1;
                flanger_.shiftInterval = flanger_.shiftMax;
            }

            if ((++flanger_.recordInterval) > (kFlangerBufferSize - 1))
                flanger_.recordInterval = 0;
            if ((++flanger_.playInterval) > (kFlangerBufferSize - 1))
                flanger_.playInterval = 0;
        }

        // phaser

        else if (type == EF_PHASER) {
            Phaser &phaser_ = effect_.phaser;

            phaser_.lfo = phaser_.depthFreq * sin(2 * M_PI * phaser_.dataY) + phaser_.centerFreq;

            phaser_.dataX += phaser_.Ts;
            phaser_.dataY = phaser_.rate * phaser_.dataX;
            if (phaser_.dataY >= 1.0)
                phaser_.dataX = 0;

            float w0 = 2 * M_PI * phaser_.lfo / kAudioSampleRate;
            float cosw0 = cos(w0);
            float sinw0 = sin(w0);
            float alpha = sinw0 / (2 * phaser_.Q);

            float b0 = 1.0 - alpha;
            float b1 = -2.0 * cosw0;
            float b2 = 1.0 + alpha;

            float a0 = 1.0 + alpha;
            float a1 = -2.0 * cosw0;
            float a2 = 1.0 - alpha;

            int32_t dataPhaser = ((b0 / a0) * input) + ((b1 / a0) * phaser_.ff[0]) + ((b2 / a0) * phaser_.ff[1]) - ((a1 / a0) * phaser_.fb[0]) - ((a2 / a0) * phaser_.fb[1]);

            int32_t data = (dataPhaser * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }

            phaser_.ff[1] = phaser_.ff[0];
            phaser_.ff[0] = input;
            phaser_.fb[1] = phaser_.fb[0];
            phaser_.fb[0] = dataPhaser;
        }

        // compressor

        else if (type == EF_COMPRESSOR) {
            Compressor &compressor_ = effect_.compressor;

            float in_float = (float)(input) / INT24_MAX;
            float in_abs = abs(in_float);
            float in_dB = 20.0 * log10(in_abs / 1.0);

            float gain;
            float gain_dB;
            float gainSmooth;

            if (in_dB < -96.0)
                in_dB = -96.0;
            if (in_dB > compressor_.threshold) {
                gain = compressor_.threshold + (in_dB - compressor_.threshold) / compressor_.rate;
            } else {
                gain = in_dB;
            }
            gain_dB = gain - in_dB;

            if (gain_dB < compressor_.gainSmoothPrev) {
                // attack mode
                gainSmooth = ((1 - compressor_.attackAlpha) * gain_dB) + (compressor_.attackAlpha * compressor_.gainSmoothPrev);
            } else {
                // release mode
                gainSmooth = ((1 - compressor_.releaseAlpha) * gain_dB) + (compressor_.releaseAlpha * compressor_.gainSmoothPrev);
            }
            compressor_.gainSmoothPrev = gainSmooth;

            float amp = pow(10.0, gainSmooth / 20.0);

            int32_t data =
                (amp * input * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }
        }

        // expander

        else if (type == EF_EXPANDER) {
            Expander &expander_ = effect_.expander;

            float in_float = (float)(input) / INT24_MAX;
            float in_abs = abs(in_float);
            float in_dB = 20.0 * log10(in_abs / 1.0);

            float gain;
            float gain_dB;
            float gainSmooth;

            if (in_dB < -144.0)
                in_dB = -144.0;
            if (in_dB > expander_.threshold) {
                gain = in_dB;
            } else {
                gain = expander_.threshold + (in_dB - expander_.threshold) * expander_.rate;
            }
            gain_dB = gain - in_dB;

            if (gain_dB > expander_.gainSmoothPrev) {
                // attack mode
                gainSmooth = ((1 - expander_.attackAlpha) * gain_dB) + (expander_.attackAlpha * expander_.gainSmoothPrev);
            } else {
                // release mode
                gainSmooth = ((1 - expander_.releaseAlpha) * gain_dB) + (expander_.releaseAlpha * expander_.gainSmoothPrev);
            }
            expander_.gainSmoothPrev = gainSmooth;

            float amp = pow(10.0, gainSmooth / 20.0);

            int32_t data =
                (amp * input * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }
        }

        // overdrive

        else if (type == EF_OVERDRIVE) {
            Overdrive &overdrive_ = effect_.overdrive;

            double in_double = ((double)(input) / INT24_MAX) * overdrive_.gain;
            double out_double;
            if (in_double < -overdrive_.threshold) {
                out_double = -2 * overdrive_.threshold / 3;
            } else if (in_double > overdrive_.threshold) {
                out_double = 2 * overdrive_.threshold / 3;
            } else {
                out_double = in_double - (in_double * in_double * in_double) / 3;
            }

            if (out_double > 1.0)
                out_double = 1.0;
            if (out_double < -1.0)
                out_double = -1.0;

            int32_t filterInput = out_double * INT24_MAX;
            int32_t filterOutput;

            overdrive_.dataIn[0] = filterInput;
            overdrive_.dataOut[0] = (overdrive_.a0 * overdrive_.dataIn[0]) + (overdrive_.a1 * overdrive_.dataIn[1]) +
                                    (overdrive_.a2 * overdrive_.dataIn[2]) - (overdrive_.b1 * overdrive_.dataOut[1]) - (overdrive_.b2 * overdrive_.dataOut[2]);

            overdrive_.dataIn[2] = overdrive_.dataIn[1];
            overdrive_.dataIn[1] = overdrive_.dataIn[0];

            overdrive_.dataOut[2] = overdrive_.dataOut[1];
            overdrive_.dataOut[1] = overdrive_.dataOut[0];

            filterOutput = overdrive_.dataOut[0];

            int32_t data =
                (filterOutput * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }
        }

        // distortion

        else if (type == EF_DISTORTION) {
            Distortion &distortion_ = effect_.distortion;

            float in_float = ((float)(input) / INT24_MAX) * distortion_.gain;

            // float out_float = (2.0 / M_PI) * atan(in_float) *
            // distortion_.threshold; float out_float = copysign(1.0, in_float) * (1 -
            // exp(-abs(distortion_.gain * in_float))) * distortion_.threshold; float
            // out_float = tanh(distortion_.gain * in_float) * distortion_.threshold;
            // float out_float = (2.0f / distortion_.gain) * tanhf(distortion_.gain *
            // in_float) * distortion_.threshold;
            float out_float = (2.0f / distortion_.gain) * atanf(distortion_.gain * in_float) * distortion_.threshold;

            int32_t filterInput = out_float * INT24_MAX;
            int32_t filterOutput;

            distortion_.dataIn[0] = filterInput;
            distortion_.dataOut[0] = (distortion_.a0 * distortion_.dataIn[0]) + (distortion_.a1 * distortion_.dataIn[1]) +
                                     (distortion_.a2 * distortion_.dataIn[2]) - (distortion_.b1 * distortion_.dataOut[1]) - (distortion_.b2 * distortion_.dataOut[2]);

            distortion_.dataIn[2] = distortion_.dataIn[1];
            distortion_.dataIn[1] = distortion_.dataIn[0];

            distortion_.dataOut[2] = distortion_.dataOut[1];
            distortion_.dataOut[1] = distortion_.dataOut[0];

            filterOutput = distortion_.dataOut[0];

            int32_t data =
                (filterOutput * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }
        }

        // bitcrusher

        else if (type == EF_BITCRUSHER) {
            Bitcrusher &bitcrusher_ = effect_.bitcrusher;

            bitcrusher_.sampleCounter += 1;
            if (bitcrusher_.sampleCounter >= bitcrusher_.sampleCounterMax) {
                bitcrusher_.sampleCounter = 0;

                bitcrusher_.sampleData = input;

                while ((bitcrusher_.sampleData < bitcrusher_.limitNeg) || (bitcrusher_.sampleData > bitcrusher_.limitPos)) {
                    if (bitcrusher_.sampleData > bitcrusher_.limitPos) {
                        switch (bitcrusher_.mode) {
                        case BIT_CLIP:
                            bitcrusher_.sampleData = bitcrusher_.limitPos;
                            break;

                        case BIT_FOLD:
                            bitcrusher_.sampleData =
                                bitcrusher_.limitPos -
                                (bitcrusher_.sampleData - bitcrusher_.limitPos);
                            if (bitcrusher_.sampleData < 0)
                                bitcrusher_.sampleData *= -1;
                            break;
                        }
                    } else if (bitcrusher_.sampleData < bitcrusher_.limitNeg) {
                        switch (bitcrusher_.mode) {
                        case BIT_CLIP:
                            bitcrusher_.sampleData = bitcrusher_.limitNeg;
                            break;

                        case BIT_FOLD:
                            bitcrusher_.sampleData =
                                bitcrusher_.limitNeg -
                                (bitcrusher_.sampleData - bitcrusher_.limitNeg);
                            if (bitcrusher_.sampleData > 0)
                                bitcrusher_.sampleData *= -1;
                            break;
                        }
                    }
                }
            }

            int32_t dataOut = bitcrusher_.sampleData & bitcrusher_.resModifier;

            int32_t data = (dataOut * effect_.wetFloat) + (input * effect_.dryFloat);

            if (effect_.genTransition.active) {
                output = (data * effect_.genTransition.activeWet) + (input * effect_.genTransition.activeDry);
            } else {
                output = data;
            }
        }
    } else {
        output = input;
    }

    return output;
}

void Controller::interruptTransition() {
    // system transition
    if (system.volumeTransition.active) {
        SystemVolumeTransition &vTransition = system.volumeTransition;

        switch (vTransition.actionVolume) {
        case SYS_ACTION_UP:
            if (system.volumeFloat >= vTransition.targetVolume) {
                vTransition.actionVolume = SYS_ACTION_NONE;
            } else {
                system.volumeFloat += 0.0001;
            }
            break;

        case SYS_ACTION_DOWN:
            if (system.volumeFloat <= vTransition.targetVolume) {
                vTransition.actionVolume = SYS_ACTION_NONE;
            } else {
                system.volumeFloat -= 0.0001;
            }
            break;
        }

        if (vTransition.actionVolume == SYS_ACTION_NONE) {
            vTransition.active = false;
        }
    }

    if (system.panTransition.active) {
        SystemPanTransition &pTransition = system.panTransition;

        switch (pTransition.actionVolumeLeft) {
        case SYS_ACTION_UP:
            if (system.volumeLeftFloat >= pTransition.targetVolumeLeft) {
                pTransition.actionVolumeLeft = SYS_ACTION_NONE;
            } else {
                system.volumeLeftFloat += 0.0001;
            }
            break;

        case SYS_ACTION_DOWN:
            if (system.volumeLeftFloat <= pTransition.targetVolumeLeft) {
                pTransition.actionVolumeLeft = SYS_ACTION_NONE;
            } else {
                system.volumeLeftFloat -= 0.0001;
            }
            break;
        }

        switch (pTransition.actionVolumeRight) {
        case SYS_ACTION_UP:
            if (system.volumeRightFloat >= pTransition.targetVolumeRight) {
                pTransition.actionVolumeRight = SYS_ACTION_NONE;
            } else {
                system.volumeRightFloat += 0.0001;
            }
            break;

        case SYS_ACTION_DOWN:
            if (system.volumeRightFloat <= pTransition.targetVolumeRight) {
                pTransition.actionVolumeRight = SYS_ACTION_NONE;
            } else {
                system.volumeRightFloat -= 0.0001;
            }
            break;
        }

        if ((pTransition.actionVolumeLeft == SYS_ACTION_NONE) &&
            (pTransition.actionVolumeRight == SYS_ACTION_NONE)) {
            pTransition.active = false;
        }
    }

    // metronome transition
    if (metronome.volumeTransition.active) {
        MetronomeVolumeTransition &vTransition = metronome.volumeTransition;

        switch (vTransition.actionVolume) {
        case MET_ACTION_UP:
            if (metronome.volumeFloat >= vTransition.targetVolume) {
                vTransition.actionVolume = MET_ACTION_NONE;
            } else {
                metronome.volumeFloat += 0.0001;
            }
            break;

        case MET_ACTION_DOWN:
            if (metronome.volumeFloat <= vTransition.targetVolume) {
                vTransition.actionVolume = MET_ACTION_NONE;
            } else {
                metronome.volumeFloat -= 0.0001;
            }
            break;
        }

        if (vTransition.actionVolume == MET_ACTION_NONE) {
            vTransition.active = false;
        }
    }

    // eq transition
    if (eq.genTransition.active) {
        EqGenTransition &gTransition = eq.genTransition;

        switch (gTransition.actionDry) {
        case EQ_ACTION_UP:
            if (gTransition.activeDry >= gTransition.targetDry) {
                gTransition.actionDry = EQ_ACTION_NONE;
            } else {
                gTransition.activeDry += 0.0001;
            }
            break;

        case EQ_ACTION_DOWN:
            if (gTransition.activeDry <= gTransition.targetDry) {
                gTransition.actionDry = EQ_ACTION_NONE;
            } else {
                gTransition.activeDry -= 0.0001;
            }
            break;
        }

        switch (gTransition.actionWet) {
        case EQ_ACTION_UP:
            if (gTransition.activeWet >= gTransition.targetWet) {
                gTransition.actionWet = EQ_ACTION_NONE;
            } else {
                gTransition.activeWet += 0.0001;
            }
            break;

        case EQ_ACTION_DOWN:
            if (gTransition.activeWet <= gTransition.targetWet) {
                gTransition.actionWet = EQ_ACTION_NONE;
            } else {
                gTransition.activeWet -= 0.0001;
            }
            break;
        }

        if ((gTransition.actionDry == EQ_ACTION_NONE) &&
            (gTransition.actionWet == EQ_ACTION_NONE)) {
            switch (gTransition.mode) {
            case EQ_MODE_NONE:
                gTransition.active = false;
                gTransition.mode = EQ_MODE_NONE;
                gTransition.phase = EQ_PHASE_NONE;
                transitionClearFlag = true;
                break;

            case EQ_MODE_ACTIVE:
                gTransition.active = false;
                gTransition.mode = EQ_MODE_NONE;
                gTransition.phase = EQ_PHASE_NONE;
                transitionClearFlag = true;
                break;
            }
        }
    }

    // filter transition
    for (uint8_t i = 0; i < kFilterLibrarySize; i++) {
        if (filter[i].genTransition.active) {
            FilterGenTransition &gTransition = filter[i].genTransition;

            switch (gTransition.actionDry) {
            case FIL_ACTION_UP:
                if (gTransition.activeDry >= gTransition.targetDry) {
                    gTransition.actionDry = FIL_ACTION_NONE;
                } else {
                    gTransition.activeDry += 0.0001;
                }
                break;

            case FIL_ACTION_DOWN:
                if (gTransition.activeDry <= gTransition.targetDry) {
                    gTransition.actionDry = FIL_ACTION_NONE;
                } else {
                    gTransition.activeDry -= 0.0001;
                }
                break;
            }

            switch (gTransition.actionWet) {
            case FIL_ACTION_UP:
                if (gTransition.activeWet >= gTransition.targetWet) {
                    gTransition.actionWet = FIL_ACTION_NONE;
                } else {
                    gTransition.activeWet += 0.0001;
                }
                break;

            case FIL_ACTION_DOWN:
                if (gTransition.activeWet <= gTransition.targetWet) {
                    gTransition.actionWet = FIL_ACTION_NONE;
                } else {
                    gTransition.activeWet -= 0.0001;
                }
                break;
            }

            if ((gTransition.actionDry == FIL_ACTION_NONE) &&
                (gTransition.actionWet == FIL_ACTION_NONE)) {
                switch (gTransition.mode) {
                case FIL_MODE_NONE:
                    gTransition.active = false;
                    gTransition.mode = FIL_MODE_NONE;
                    gTransition.phase = FIL_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case FIL_MODE_ACTIVE:
                    gTransition.active = false;
                    gTransition.mode = FIL_MODE_NONE;
                    gTransition.phase = FIL_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case FIL_MODE_TYPE:
                    switch (gTransition.phase) {
                    case EF_PHASE_NONE:
                        gTransition.active = false;
                        gTransition.mode = FIL_MODE_NONE;
                        gTransition.phase = FIL_PHASE_NONE;
                        transitionClearFlag = true;
                        break;

                    case EF_PHASE_A:
                        filter[i].calculateCoef();

                        gTransition.phase = FIL_PHASE_B;

                        gTransition.activeDry = 1.0;
                        gTransition.targetDry = 0.0;

                        gTransition.activeWet = 0.0;
                        gTransition.targetWet = 1.0;

                        filter_calculateGenTransition(i);

                        transitionShowFlag = 2;
                        break;

                    case EF_PHASE_B:
                        gTransition.active = false;
                        gTransition.mode = FIL_MODE_NONE;
                        gTransition.phase = FIL_PHASE_NONE;
                        transitionClearFlag = true;
                        break;
                    }
                    break;
                }
            }
        }

        if (filter[i].mixTransition.active) {
            Filter &filter_ = filter[i];
            FilterMixTransition &mTransition = filter[i].mixTransition;

            switch (mTransition.actionDry) {
            case FIL_ACTION_UP:
                if (filter_.dryFloat >= mTransition.targetDry) {
                    mTransition.actionDry = FIL_ACTION_NONE;
                } else {
                    filter_.dryFloat += 0.0001;
                }
                break;

            case FIL_ACTION_DOWN:
                if (filter_.dryFloat <= mTransition.targetDry) {
                    mTransition.actionDry = FIL_ACTION_NONE;
                } else {
                    filter_.dryFloat -= 0.0001;
                }
                break;
            }

            switch (mTransition.actionWet) {
            case FIL_ACTION_UP:
                if (filter_.wetFloat >= mTransition.targetWet) {
                    mTransition.actionWet = FIL_ACTION_NONE;
                } else {
                    filter_.wetFloat += 0.0001;
                }
                break;

            case FIL_ACTION_DOWN:
                if (filter_.wetFloat <= mTransition.targetWet) {
                    mTransition.actionWet = FIL_ACTION_NONE;
                } else {
                    filter_.wetFloat -= 0.0001;
                }
                break;
            }

            if ((mTransition.actionDry == FIL_ACTION_NONE) &&
                (mTransition.actionWet == FIL_ACTION_NONE)) {
                mTransition.active = false;
            }
        }
    }

    // effect transition
    for (uint8_t i = 0; i < kEffectLibrarySize; i++) {
        if (effect[i].genTransition.active) {
            EffectGenTransition &gTransition = effect[i].genTransition;

            switch (gTransition.actionDry) {
            case EF_ACTION_UP:
                if (gTransition.activeDry >= gTransition.targetDry) {
                    gTransition.actionDry = EF_ACTION_NONE;
                } else {
                    gTransition.activeDry += 0.0001;
                }
                break;

            case EF_ACTION_DOWN:
                if (gTransition.activeDry <= gTransition.targetDry) {
                    gTransition.actionDry = EF_ACTION_NONE;
                } else {
                    gTransition.activeDry -= 0.0001;
                }
                break;
            }

            switch (gTransition.actionWet) {
            case EF_ACTION_UP:
                if (gTransition.activeWet >= gTransition.targetWet) {
                    gTransition.actionWet = EF_ACTION_NONE;
                } else {
                    gTransition.activeWet += 0.0001;
                }
                break;

            case EF_ACTION_DOWN:
                if (gTransition.activeWet <= gTransition.targetWet) {
                    gTransition.actionWet = EF_ACTION_NONE;
                } else {
                    gTransition.activeWet -= 0.0001;
                }
                break;
            }

            switch (gTransition.actionRecordWet) {
            case EF_ACTION_UP:
                if (gTransition.activeRecordWet >= gTransition.targetRecordWet) {
                    gTransition.actionRecordWet = EF_ACTION_NONE;
                } else {
                    gTransition.activeRecordWet += 0.0001;
                }
                break;

            case EF_ACTION_DOWN:
                if (gTransition.activeRecordWet <= gTransition.targetRecordWet) {
                    gTransition.actionRecordWet = EF_ACTION_NONE;
                } else {
                    gTransition.activeRecordWet -= 0.0001;
                }
                break;
            }

            if ((gTransition.actionDry == EF_ACTION_NONE) && (gTransition.actionWet == EF_ACTION_NONE) && (gTransition.actionRecordWet == EF_ACTION_NONE)) {
                switch (gTransition.mode) {
                case EF_MODE_NONE:
                    gTransition.active = false;
                    gTransition.mode = EF_MODE_NONE;
                    gTransition.phase = EF_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case EF_MODE_ACTIVE:
                    gTransition.active = false;
                    gTransition.mode = EF_MODE_NONE;
                    gTransition.phase = EF_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case EF_MODE_TYPE:
                case EF_MODE_TIME:
                case EF_MODE_FEEDBACK:
                    switch (gTransition.phase) {
                    case EF_PHASE_NONE:
                        gTransition.active = false;
                        gTransition.mode = EF_MODE_NONE;
                        gTransition.phase = EF_PHASE_NONE;
                        transitionClearFlag = true;
                        break;

                    case EF_PHASE_A:
                        switch (gTransition.mode) {
                        case EF_MODE_TYPE:
                            gTransition.activeType = gTransition.targetType;
                            break;

                        case EF_MODE_TIME:
                            switch (effect[i].type) {
                            case EF_DELAY:
                                effect[i].delay.time = kDelayTimeDataLibrary[effect[i].delay.aTime].data;
                                effect[i].delay.update(rhythm.tempo);
                                break;

                            case EF_CHORUS:
                                effect[i].chorus.time = kChorusTimeDataLibrary[effect[i].chorus.aTime].data;
                                effect[i].chorus.update();
                                break;

                            case EF_FLANGER:
                                effect[i].flanger.time = kFlangerTimeDataLibrary[effect[i].flanger.aTime].data;
                                effect[i].flanger.update();
                                break;
                            }
                            break;

                        case EF_MODE_FEEDBACK:
                            switch (effect[i].type) {
                            case EF_CHORUS:
                                effect[i].chorus.feedback = kChorusFeedbackDataLibrary[effect[i].chorus.bFeedback].data;
                                effect[i].chorus.update();
                                break;

                            case EF_FLANGER:
                                effect[i].flanger.feedback = kFlangerFeedbackDataLibrary[effect[i].flanger.bFeedback].data;
                                effect[i].flanger.update();
                                break;
                            }
                            break;
                        }

                        gTransition.phase = EF_PHASE_B;

                        gTransition.activeDry = 1.0;
                        gTransition.targetDry = 0.0;

                        gTransition.activeWet = 0.0;
                        gTransition.targetWet = 1.0;

                        gTransition.activeRecordWet = 0.0;
                        gTransition.targetRecordWet = 1.0;

                        effect_calculateGenTransition(i);

                        transitionShowFlag = 2;
                        break;

                    case EF_PHASE_B:
                        gTransition.active = false;
                        gTransition.mode = EF_MODE_NONE;
                        gTransition.phase = EF_PHASE_NONE;
                        transitionClearFlag = true;
                        break;
                    }
                    break;
                }
            }
        }

        if (effect[i].mixTransition.active) {
            Effect &effect_ = effect[i];
            EffectMixTransition &mTransition = effect[i].mixTransition;

            switch (mTransition.actionDry) {
            case EF_ACTION_UP:
                if (effect_.dryFloat >= mTransition.targetDry) {
                    mTransition.actionDry = EF_ACTION_NONE;
                } else {
                    effect_.dryFloat += 0.0001;
                }
                break;

            case EF_ACTION_DOWN:
                if (effect_.dryFloat <= mTransition.targetDry) {
                    mTransition.actionDry = EF_ACTION_NONE;
                } else {
                    effect_.dryFloat -= 0.0001;
                }
                break;
            }

            switch (mTransition.actionWet) {
            case EF_ACTION_UP:
                if (effect_.wetFloat >= mTransition.targetWet) {
                    mTransition.actionWet = EF_ACTION_NONE;
                } else {
                    effect_.wetFloat += 0.0001;
                }
                break;

            case EF_ACTION_DOWN:
                if (effect_.wetFloat <= mTransition.targetWet) {
                    mTransition.actionWet = EF_ACTION_NONE;
                } else {
                    effect_.wetFloat -= 0.0001;
                }
                break;
            }

            if ((mTransition.actionDry == EF_ACTION_NONE) && (mTransition.actionWet == EF_ACTION_NONE)) {
                mTransition.active = false;
            }
        }
    }

    // reverb transition
    if (reverb.genTransition.active) {
        ReverbGenTransition &gTransition = reverb.genTransition;

        switch (gTransition.actionDry) {
        case REV_ACTION_UP:
            if (gTransition.activeDry >= gTransition.targetDry) {
                gTransition.actionDry = REV_ACTION_NONE;
            } else {
                gTransition.activeDry += 0.0001;
            }
            break;

        case REV_ACTION_DOWN:
            if (gTransition.activeDry <= gTransition.targetDry) {
                gTransition.actionDry = REV_ACTION_NONE;
            } else {
                gTransition.activeDry -= 0.0001;
            }
            break;
        }

        switch (gTransition.actionWet) {
        case REV_ACTION_UP:
            if (gTransition.activeWet >= gTransition.targetWet) {
                gTransition.actionWet = REV_ACTION_NONE;
            } else {
                gTransition.activeWet += 0.0001;
            }
            break;

        case REV_ACTION_DOWN:
            if (gTransition.activeWet <= gTransition.targetWet) {
                gTransition.actionWet = REV_ACTION_NONE;
            } else {
                gTransition.activeWet -= 0.0001;
            }
            break;
        }

        if ((gTransition.actionDry == REV_ACTION_NONE) && (gTransition.actionWet == REV_ACTION_NONE)) {
            switch (gTransition.mode) {
            case REV_MODE_NONE:
                gTransition.active = false;
                gTransition.mode = REV_MODE_NONE;
                gTransition.phase = REV_PHASE_NONE;
                transitionClearFlag = true;
                break;

            case REV_MODE_ACTIVE:
                gTransition.active = false;
                gTransition.mode = REV_MODE_NONE;
                gTransition.phase = REV_PHASE_NONE;
                transitionClearFlag = true;
                break;

            case REV_MODE_PREDELAY:
            case REV_MODE_SURROUND:
                switch (gTransition.phase) {
                case REV_PHASE_NONE:
                    gTransition.active = false;
                    gTransition.mode = REV_MODE_NONE;
                    gTransition.phase = REV_PHASE_NONE;
                    transitionClearFlag = true;
                    break;

                case REV_PHASE_A:
                    switch (gTransition.mode) {
                    case REV_MODE_PREDELAY:
                        reverb.setPreDelay(kReverbPreDelayDataLibrary[reverb.preDelay].data);
                        break;

                    case REV_MODE_SURROUND:
                        reverb.setSurround(kReverbSurroundDataLibrary[reverb.surround].data);
                        break;
                    }
                    gTransition.phase = REV_PHASE_B;

                    gTransition.activeDry = 1.0;
                    gTransition.targetDry = 0.0;

                    gTransition.activeWet = 0.0;
                    gTransition.targetWet = 1.0;

                    reverb_calculateGenTransition();

                    transitionShowFlag = 2;
                    break;

                case REV_PHASE_B:
                    gTransition.active = false;
                    gTransition.mode = REV_MODE_NONE;
                    gTransition.phase = REV_PHASE_NONE;
                    transitionClearFlag = true;
                    break;
                }
                break;
            }
        }
    }

    if (reverb.mixTransition.active) {
        ReverbMixTransition &mTransition = reverb.mixTransition;

        switch (mTransition.actionDry) {
        case REV_ACTION_UP:
            if (reverb.dryFloat >= mTransition.targetDry) {
                mTransition.actionDry = REV_ACTION_NONE;
            } else {
                reverb.dryFloat += 0.0001;
            }
            break;

        case REV_ACTION_DOWN:
            if (reverb.dryFloat <= mTransition.targetDry) {
                mTransition.actionDry = REV_ACTION_NONE;
            } else {
                reverb.dryFloat -= 0.0001;
            }
            break;
        }

        switch (mTransition.actionWet) {
        case REV_ACTION_UP:
            if (reverb.wetFloat >= mTransition.targetWet) {
                mTransition.actionWet = REV_ACTION_NONE;
            } else {
                reverb.wetFloat += 0.0001;
            }
            break;

        case REV_ACTION_DOWN:
            if (reverb.wetFloat <= mTransition.targetWet) {
                mTransition.actionWet = REV_ACTION_NONE;
            } else {
                reverb.wetFloat -= 0.0001;
            }
            break;
        }

        if ((mTransition.actionDry == REV_ACTION_NONE) && (mTransition.actionWet == REV_ACTION_NONE)) {
            mTransition.active = false;
        }
    }
}

void Controller::interruptLeftButtonTrigger() {
    keyboard.leftButtonState = PREWAIT;
    keyboard.leftButtonCounter = 0;
    startLeftButtonTimer();
}

void Controller::interruptRightButtonTrigger() {
    keyboard.rightButtonState = PREWAIT;
    keyboard.rightButtonCounter = 0;
    startRightButtonTimer();
}

void Controller::interruptBeatButtonTrigger() {
    keyboard.beatButtonState = PREWAIT;
    keyboard.beatButtonCounter = 0;
    startBeatButtonTimer();
}

void Controller::interruptLeftButtonRead() {
    switch (keyboard.leftButtonState) {
    case PASSIVE:
        break;

    case PREWAIT:
        if (keyboard.leftButtonCounter < 50) {
            keyboard.leftButtonCounter += 1;
        } else {
            keyboard.leftButtonState = READ;
            keyboard.leftButtonCounter = 0;
        }
        break;

    case READ:
        if (keyboard.leftButtonCounter < 32) {
            switch (keyboard.leftButtonCounter % 2) {
            case 0:
                CT0_SCL_LOW;
                break;

            case 1:
                bool value = CT0_SDO_READ;
                if (!value) {
                    keyboard.leftButtonTemp = (keyboard.leftButtonCounter / 2) + 1;
                }
                CT0_SCL_HIGH;
                break;
            }
            keyboard.leftButtonCounter += 1;
        } else {
            keyboard.leftButtonState = POSTWAIT;
            keyboard.leftButtonCounter = 0;
        }
        break;

    case POSTWAIT:
        if (keyboard.leftButtonCounter < 1000) {
            keyboard.leftButtonCounter += 1;
        } else {
            keyboard.leftButtonState = PASSIVE;
            keyboard.leftButtonCounter = 0;
            keyboard.leftButton = keyboard.leftButtonTemp;
            keyboard.leftButtonTemp = 0;
            stopLeftButtonTimer();
        }
        break;

    default:
        break;
    }
}

void Controller::interruptRightButtonRead() {
    switch (keyboard.rightButtonState) {
    case PASSIVE:
        break;

    case PREWAIT:
        if (keyboard.rightButtonCounter < 50) {
            keyboard.rightButtonCounter += 1;
        } else {
            keyboard.rightButtonState = READ;
            keyboard.rightButtonCounter = 0;
        }
        break;

    case READ:
        if (keyboard.rightButtonCounter < 32) {
            switch (keyboard.rightButtonCounter % 2) {
            case 0:
                CT1_SCL_LOW;
                break;

            case 1:
                bool value = CT1_SDO_READ;
                if (!value) {
                    keyboard.rightButtonTemp = (keyboard.rightButtonCounter / 2) + 1;
                }
                CT1_SCL_HIGH;
                break;
            }
            keyboard.rightButtonCounter += 1;
        } else {
            keyboard.rightButtonState = POSTWAIT;
            keyboard.rightButtonCounter = 0;
        }
        break;

    case POSTWAIT:
        if (keyboard.rightButtonCounter < 1000) {
            keyboard.rightButtonCounter += 1;
        } else {
            keyboard.rightButtonState = PASSIVE;
            keyboard.rightButtonCounter = 0;
            keyboard.rightButton = keyboard.rightButtonTemp;
            keyboard.rightButtonTemp = 0;
            stopRightButtonTimer();
        }
        break;

    default:
        break;
    }
}

void Controller::interruptBeatButtonRead() {
    switch (keyboard.beatButtonState) {
    case PASSIVE:
        break;

    case PREWAIT:
        if (keyboard.beatButtonCounter < 50) {
            keyboard.beatButtonCounter += 1;
        } else {
            keyboard.beatButtonState = READ;
            keyboard.beatButtonCounter = 0;
        }
        break;

    case READ:
        if (keyboard.beatButtonCounter < 32) {
            switch (keyboard.beatButtonCounter % 2) {
            case 0:
                CT2_SCL_LOW;
                break;

            case 1:
                bool value = CT2_SDO_READ;
                if (!value) {
                    keyboard.beatButtonTemp = (keyboard.beatButtonCounter / 2) + 1;
                }
                CT2_SCL_HIGH;
                break;
            }
            keyboard.beatButtonCounter += 1;
        } else {
            keyboard.beatButtonState = POSTWAIT;
            keyboard.beatButtonCounter = 0;
        }
        break;

    case POSTWAIT:
        if (keyboard.beatButtonCounter < 1000) {
            keyboard.beatButtonCounter += 1;
        } else {
            keyboard.beatButtonState = PASSIVE;
            keyboard.beatButtonCounter = 0;
            keyboard.beatButton = keyboard.beatButtonTemp;
            keyboard.beatButtonTemp = 0;
            stopBeatButtonTimer();
        }
        break;

    default:
        break;
    }
}

void Controller::interruptUpDownButtonRead() {
    if (upDownButtonCounter < 2) {
        upDownButtonCounter += 1;
    } else {
        if (!alertFlag) {
            if (upButtonFlag) {
                switch (menu) {
                case FILE_MENU:
                    file_menuUp();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuUp();
                    break;

                case SYSTEM_MENU:
                    if ((menuTab != 3) && (menuTab != 6) && (menuTab != 7)) {
                        system_menuUp();
                    }
                    break;

                case RHYTHM_MENU:
                    rhythm_menuUp();
                    break;

                case METRO_MENU:
                    if ((menuTab != 0) && (menuTab != 1)) {
                        metro_menuUp();
                    }
                    break;

                case EQ_MENU:
                    if (menuTab != 0) {
                        eq_menuUp();
                    }
                    break;

                case OSC_A0_MENU:
                    if ((menuTab != 0) && (menuTab != 2) && (menuTab != 7)) {
                        osc_menuUp(osc[0]);
                    }
                    break;

                case OSC_A1_MENU:
                    if ((menuTab != 6) && (menuTab != 7)) {
                        osc_menuUp(osc[0]);
                    }
                    break;

                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    if ((menuTab != 0) && (menuTab != 4) && (menuTab != 7)) {
                        osc_menuUp(osc[0]);
                    }
                    break;

                case OSC_B0_MENU:
                    if ((menuTab != 0) && (menuTab != 2) && (menuTab != 7)) {
                        osc_menuUp(osc[1]);
                    }
                    break;

                case OSC_B1_MENU:
                    if ((menuTab != 6) && (menuTab != 7)) {
                        osc_menuUp(osc[1]);
                    }
                    break;

                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    if ((menuTab != 0) && (menuTab != 4) && (menuTab != 7)) {
                        osc_menuUp(osc[1]);
                    }
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    if ((menuTab != 0) && (menuTab != 5)) {
                        filter_menuUp();
                    }
                    break;

                case ENVELOPE_MENU:
                    if (menuTab != 0) {
                        envelope_menuUp();
                    }
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    if (menuTab != 0) {
                        effect_menuUp();
                    }
                    break;

                case REVERB_MENU:
                    if (menuTab != 0) {
                        reverb_menuUp();
                    }
                    break;

                case KEY_MENU:
                    if ((menuTab != 2) && (menuTab != 7)) {
                        key_menuUp();
                    }
                    break;

                case SONG_MENU:
                    song_menuUp();
                    break;
                }
            } else if (downButtonFlag) {
                switch (menu) {
                case FILE_MENU:
                    file_menuDown();
                    break;

                case SYNTHKIT_MENU:
                    synthkit_menuDown();
                    break;

                case SYSTEM_MENU:
                    if ((menuTab != 3) && (menuTab != 6) && (menuTab != 7)) {
                        system_menuDown();
                    }
                    break;

                case RHYTHM_MENU:
                    rhythm_menuDown();
                    break;

                case METRO_MENU:
                    if ((menuTab != 0) && (menuTab != 1)) {
                        metro_menuDown();
                    }
                    break;

                case EQ_MENU:
                    if (menuTab != 0) {
                        eq_menuDown();
                    }
                    break;

                case OSC_A0_MENU:
                    if ((menuTab != 0) && (menuTab != 2) && (menuTab != 7)) {
                        osc_menuDown(osc[0]);
                    }
                    break;

                case OSC_A1_MENU:
                    if ((menuTab != 6) && (menuTab != 7)) {
                        osc_menuDown(osc[0]);
                    }
                    break;

                case OSC_A2_MENU:
                case OSC_A3_MENU:
                    if ((menuTab != 0) && (menuTab != 4) && (menuTab != 7)) {
                        osc_menuDown(osc[0]);
                    }
                    break;

                case OSC_B0_MENU:
                    if ((menuTab != 0) && (menuTab != 2) && (menuTab != 7)) {
                        osc_menuDown(osc[1]);
                    }
                    break;

                case OSC_B1_MENU:
                    if ((menuTab != 6) && (menuTab != 7)) {
                        osc_menuDown(osc[1]);
                    }
                    break;

                case OSC_B2_MENU:
                case OSC_B3_MENU:
                    if ((menuTab != 0) && (menuTab != 4) && (menuTab != 7)) {
                        osc_menuDown(osc[1]);
                    }
                    break;

                case FILTER_0_MENU:
                case FILTER_1_MENU:
                    if ((menuTab != 0) && (menuTab != 5)) {
                        filter_menuDown();
                    }
                    break;

                case ENVELOPE_MENU:
                    if (menuTab != 0) {
                        envelope_menuDown();
                    }
                    break;

                case EFFECT_0_MENU:
                case EFFECT_1_MENU:
                    if (menuTab != 0) {
                        effect_menuDown();
                    }
                    break;

                case REVERB_MENU:
                    if (menuTab != 0) {
                        reverb_menuDown();
                    }
                    break;

                case KEY_MENU:
                    if ((menuTab != 2) && (menuTab != 7)) {
                        key_menuDown();
                    }
                    break;

                case SONG_MENU:
                    song_menuDown();
                    break;
                }
            }
        }
    }
}

void Controller::interruptLongButtonRead() {
    // main select
    if (mainMenuButtonFlag) {
        if (keyboard.longButtonCounter < kLongButtonCountLow) {
            keyboard.longButtonCounter += 1;
        } else {
            stopLongButtonTimer();
            keyboard.longButtonCounter = 0;
            mainMenuButtonFlag = false;
            main_select();
        }
    } else {
        if (keyboard.longButtonCounter < kLongButtonCountHigh) {
            keyboard.longButtonCounter += 1;
        } else {
            // song beat
            if (songBeatButtonFlag) {
                keyboard.longButtonCounter = 0;
                switch (songBeatButtonStage) {
                case 0:
                    song_generateBeat(0);
                    songBeatButtonStage = 1;
                    break;

                case 1:
                    song_generateBeat(1);
                    songBeatButtonStage = 2;
                    break;

                case 2:
                    song_generateBeat(2);
                    songBeatButtonFlag = false;
                    stopLongButtonTimer();
                    keyboard.longButtonCounter = 0;
                    break;

                default:
                    break;
                }
            }

            // song clear
            else if (songClearButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                songClearButtonFlag = false;
                song_resetAllBeats(activeBankNum);
            }

            // song copy
            else if (songCopyButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                noteCopyButtonFlag = false;
                songCopyButtonFlag = false;
                if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) {
                    copyBankSongNum = activeBankNum;
                    textCopyFlag = true;
                }
            }

            // song paste
            else if (songPasteButtonFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                notePasteButtonFlag = false;
                songPasteButtonFlag = false;
                if ((copyBankSongNum != -1) && (song.bankLibrary[activeBankNum].lastActiveBeatNum == -1) && (song.bankLibrary[copyBankSongNum].lastActiveBeatNum != -1)) {
                    for (uint8_t i = 0; i <= song.bankLibrary[copyBankSongNum].lastActiveBeatNum; i++) {
                        song.bankLibrary[activeBankNum].beatLibrary[i] = song.bankLibrary[copyBankSongNum].beatLibrary[i];
                    }
                    song_calculateLastActiveBeatNum(activeBankNum);
                    song_calculateNextBeatNum(activeBankNum, playInterval);

                    lcd_drawSong(activeBankNum);

                    if (menu == SONG_MENU) {
                        (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1) ? selectedBeatNum = 0 : selectedBeatNum = -1;
                        if (song.bankLibrary[activeBankNum].lastActiveBeatNum != -1)
                            lcd_drawBeat(activeBankNum, 0, true);
                        lcd_drawSong_BeatData();
                        lcd_drawSong_BeatGraph();
                    }

                    textPasteFlag = true;
                }
            }

            // rhythm unlock
            else if (rhythmUnlockFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                rhythmUnlockFlag = false;
                rhythm.measureLock = false;
                rhythm.barLock = false;
                rhythm.quantizeLock = false;
                lcd_drawRhythm_MeasureData();
                lcd_drawRhythm_BarData();
                lcd_drawRhythm_QuantizeData();
            }

            // rhythm lock
            else if (rhythmLockFlag) {
                stopLongButtonTimer();
                keyboard.longButtonCounter = 0;
                rhythm.measureLock = true;
                rhythm.barLock = true;
                rhythm.quantizeLock = true;
                lcd_drawRhythm_MeasureData();
                lcd_drawRhythm_BarData();
                lcd_drawRhythm_QuantizeData();
            }
        }
    }
}

void Controller::interruptPowerButtonRead() {
    if (powerButtonFlag) {
        if (powerButtonCounter < 3) {
            powerButtonCounter += 1;
        } else {
            stopPowerButtonTimer();
            powerButtonFlag = false;
            powerButtonCounter = 0;
            (!power) ? power = true : power = false;
        }
    }
}

void Controller::interruptText() {
    if (textShow) {
        textShow = false;
    } else {
        stopTextTimer();
        textClearFlag = true;
    }
}

void Controller::interruptSd() {
    if ((sd.detect) && (sd_detect() == SD_ERROR)) {
        f_close(&sd.file);
        sd_unmount();
        if (playActive)
            triggerReset();
        sd.detect = false;
        sd.ready = false;
        lcd_drawSdAlert(SD_ERROR_DETECT);
        lcd_drawSdData();
    }
}

void Controller::interruptBeatSync() {
    SYNC_OUT_OFF;
    stopBeatSyncTimer();
}

void Controller::interruptLimitAlert() {
    stopLimitAlertTimer();
    limitAlertClearFlag = true;
}

/*
void Controller::writeWavData() {
    f_open(&sd.file, "Record.wav", FA_WRITE | FA_CREATE_ALWAYS);
    f_write(&sd.file, wavIntro, sizeof(wavIntro), &sd.byteswritten);
    f_write(&sd.file, &wavData[0], 176400, &sd.byteswritten);
    f_close(&sd.file);
}
*/
