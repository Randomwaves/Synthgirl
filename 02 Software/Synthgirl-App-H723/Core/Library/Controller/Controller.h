#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <cctype>

#include "Dac.h"
#include "Global.h"
#include "Lcd.h"
#include "Lfo.h"
#include "fatfs.h"
#include "main.h"
#include "sdmmc.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" uint8_t syncInData[2];
extern "C" uint8_t syncOutData[2];

extern "C" uint8_t midiRxData;

extern "C" UART_HandleTypeDef huart4; // sync-out
extern "C" UART_HandleTypeDef huart7; // sync-in

extern "C" UART_HandleTypeDef huart1; // midi-rx
extern "C" UART_HandleTypeDef huart6; // midi-tx

extern "C" TIM_HandleTypeDef htim5;  // transition timer
extern "C" TIM_HandleTypeDef htim6;  // left button timer
extern "C" TIM_HandleTypeDef htim7;  // right button timer
extern "C" TIM_HandleTypeDef htim8;  // beat button timer
extern "C" TIM_HandleTypeDef htim12; // updown long press timer
extern "C" TIM_HandleTypeDef htim13; // button long press timer
extern "C" TIM_HandleTypeDef htim14; // play timer
extern "C" TIM_HandleTypeDef htim15; // text timer
extern "C" TIM_HandleTypeDef htim16; // power button timer
extern "C" TIM_HandleTypeDef htim17; // sd check timer
extern "C" TIM_HandleTypeDef htim23; // beat sync timer
extern "C" TIM_HandleTypeDef htim24; // limit alert timer

class Controller {
private:
bool recordBeat;

    Sys system;
    Rhythm rhythm;
    Metronome metronome;
    Lpf lpf;
    Eq eq;
    Osc osc[kOscLibrarySize];
    Filter filter[kFilterLibrarySize];
    Envelope envelope;
    Effect effect[kEffectLibrarySize];
    Reverb reverb;

    Key key;
    Song song;

    MetroPlayData mD;
    SongPlayData sD;

    int32_t audioMetronome; // 32-bit
    int32_t audioSong;      // 24-bit
    int32_t audioLpf;       // 24-bit
    int32_t audioEq;        // 24-bit
    int32_t audioFilter;    // 24-bit
    int32_t audioEffect;    // 24-bit
    int32_t audioReverb_L;  // 32-bit
    int32_t audioReverb_R;  // 32-bit
    int32_t audioSend_L;    // 32-bit
    int32_t audioSend_R;    // 32-bit

    int8_t selectedBeatNum;
    int8_t recordNoteNum;
    int8_t recordBeatNum;

    bool notePressed;
    int8_t notePressedBeatNum;

    uint8_t activeBankNum;
    uint8_t targetBankNum;
    bool bankShiftFlag;
    bool bankActionFlag;

    uint8_t activeOctaveNum;

    Menu menu;
    Menu preMenu;
    int8_t menuTab;
    int8_t preMenuTab;

    int8_t subMenuTab;

    bool sdInsertCheck;

    uint16_t fileLibrarySize;
    uint16_t synthkitLibrarySize;
    uint16_t wavetableLibrarySize;

    uint16_t songInterval;
    uint16_t barInterval;
    uint16_t measureInterval;
    uint16_t playInterval;
    uint16_t stopInterval;
    uint16_t resetInterval;

    bool stopFlag;
    bool resetFlag;

    bool alertFlag;
    AlertType alertType;

    Icon resetIcon;
    Icon playIcon;
    Icon stopIcon;
    Icon recordIcon;

    bool resetPlayFlag;

    FileStatus fileStatus;
    FileStatus synthkitStatus;

    TriggerMode triggerMode;

    bool playActive;
    bool recordActive;

    uint32_t playTimerPeriod;

    uint16_t playX;
    float playXRatio;
    RGB16Color playColor;

    bool power;

    bool powerButtonFlag;
    uint8_t powerButtonCounter;

    bool mainMenuButtonFlag;

    bool upButtonFlag;
    bool downButtonFlag;
    uint8_t upDownButtonCounter;

    bool songBeatButtonFlag;
    bool songClearButtonFlag;

    bool noteCopyButtonFlag;
    bool notePasteButtonFlag;

    bool songCopyButtonFlag;
    bool songPasteButtonFlag;

    bool rhythmUnlockFlag;
    bool rhythmLockFlag;

    bool textCopyFlag;
    bool textPasteFlag;
    bool textClearFlag;

    bool limitAlertShowFlag;
    bool limitAlertClearFlag;
    bool limitAlertActive;

    int8_t copyBeatNote;
    int8_t copyBeatOctave;
    int8_t copyBankSongNum;

    uint8_t songBeatButtonStage;

    int8_t fileMenuCounter;
    int8_t synthkitMenuCounter;

    uint8_t transitionShowFlag;
    bool transitionClearFlag;

    bool animation;
    bool textShow;

    /*
    uint32_t wavCounter = 0;
    bool wavReadFlag = false;
    bool wavWriteFlag = false;
    bool wavRead = false;
    int16_t wavData[88200] = {0};
    void writeWavData();
    */

public:
    Controller();
    ~Controller();

    Lcd lcd;
    Dac dac;
    Sd sd;
    Keyboard keyboard;

    Menu getMenu() { return menu; }
    Sys getSystem() { return system; }

    bool midiTxBusy;

    bool getPower() { return power; }

    void initialize();
    void systemStart();
    void systemReset();
    void systemUpdate_A();
    void systemUpdate_B();
    bool sendMidiCommand(uint8_t command_, uint8_t data0_, uint8_t data1_);
    void receiveMidiCommand();
    bool sendSyncCommand(uint8_t data_);
    void receiveSyncCommand();
    void calculateSongInterval();
    uint16_t calculateTriggerInterval();
    void adjustMeasureBarTiming();
    void updatePlayTimerPeriod();

    /* Button functions --------------------------------------------------------*/

    void button_check();

    /* Keyboard functions ----------------------------------------------------*/

    void keyboard_initialize();
    void keyboard_check_A();
    void keyboard_check_B();
    void keyboard_check_C();
    void keyboard_enable();
    void keyboard_disable();

    void setLeftButtonState(ButtonState state_) {
        keyboard.leftButtonState = state_;
    }
    ButtonState getLeftButtonState() const {
        return keyboard.leftButtonState;
    }
    void setRightButtonState(ButtonState state_) {
        keyboard.rightButtonState = state_;
    }
    ButtonState getRightButtonState() const {
        return keyboard.rightButtonState;
    }
    void setBeatButtonState(ButtonState state_) {
        keyboard.beatButtonState = state_;
    }
    ButtonState getBeatButtonState() const {
        return keyboard.beatButtonState;
    }

    /* Timer functions -------------------------------------------------------*/

    void startTransitionTimer() {
        HAL_TIM_Base_Start_IT(&htim5);
    }
    void stopTransitionTimer() {
        HAL_TIM_Base_Stop_IT(&htim5);
        __HAL_TIM_SET_COUNTER(&htim5, 0);
    }
    void startLeftButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim6);
    }
    void stopLeftButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim6);
    }
    void startRightButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim7);
    }
    void stopRightButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim7);
    }
    void startBeatButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim8);
    }
    void stopBeatButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim8);
    }
    void startUpDownButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim12);
    }
    void stopUpDownButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim12);
        __HAL_TIM_SET_COUNTER(&htim12, 0);
    }
    void startLongButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim13);
    }
    void stopLongButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim13);
        __HAL_TIM_SET_COUNTER(&htim13, 0);
    }
    void startPlayTimer() {
        HAL_TIM_Base_Start_IT(&htim14);
    }
    void stopPlayTimer() {
        HAL_TIM_Base_Stop_IT(&htim14);
    }
    void startTextTimer() {
        HAL_TIM_Base_Start_IT(&htim15);
    }
    void stopTextTimer() {
        HAL_TIM_Base_Stop_IT(&htim15);
        __HAL_TIM_SET_COUNTER(&htim15, 0);
    }
    void startPowerButtonTimer() {
        HAL_TIM_Base_Start_IT(&htim16);
        powerButtonCounter = 0;
    }
    void stopPowerButtonTimer() {
        HAL_TIM_Base_Stop_IT(&htim16);
        __HAL_TIM_SET_COUNTER(&htim16, 0);
    }
    void startSdTimer() {
        HAL_TIM_Base_Start_IT(&htim17);
    }
    void stopSdTimer() {
        HAL_TIM_Base_Stop_IT(&htim17);
        __HAL_TIM_SET_COUNTER(&htim17, 0);
    }
    void startBeatSyncTimer() {
        HAL_TIM_Base_Start_IT(&htim23);
    }
    void stopBeatSyncTimer() {
        HAL_TIM_Base_Stop_IT(&htim23);
        __HAL_TIM_SET_COUNTER(&htim23, 0);
    }
    void startLimitAlertTimer() {
        HAL_TIM_Base_Start_IT(&htim24);
    }
    void stopLimitAlertTimer() {
        HAL_TIM_Base_Stop_IT(&htim24);
        __HAL_TIM_SET_COUNTER(&htim24, 0);
    }

    /* Dac functions ---------------------------------------------------------*/

    void dac_initialize();

    /* Sd functions ----------------------------------------------------------*/

    SdResult sd_initialize();
    SdResult sd_reinitialize();
    SdResult sd_detect();
    SdResult sd_mount();
    SdResult sd_unmount();
    SdResult sd_getLabel();
    SdResult sd_setLabel();
    SdResult sd_getSpace();
    SdResult sd_checkFileExist(char *fileAddress);
    SdResult sd_checkFolderExist(char *folderAddress);
    SdResult sd_loadImage(char *fileAddress, uint32_t paletteAddress, uint32_t dataAddress, uint16_t paletteSize, uint16_t width, uint16_t height, RGBMode mode);
    SdResult load16BitAudio(char *fileAddress, uint32_t ramAddress, uint32_t sampleSize);
    SdResult load24BitAudio(char *fileAddress, uint32_t ramAddress, uint32_t sampleSize);
    SdResult sd_checkMetronome();
    SdResult sd_loadMetronome();
    void sd_getLibraries();
    SdResult sd_getFileLibrary();
    SdResult sd_getSynthkitLibrary();
    void sd_getWavetableLibrary();
    SdResult sd_checkFile(uint8_t fileNum_);
    SdResult sd_loadFile(uint8_t fileNum_);
    SdResult sd_saveFile(uint8_t fileNum_);
    SdResult sd_clearFile(uint8_t fileNum_);
    SdResult sd_checkSynthkit(uint8_t kitNum_);
    SdResult sd_loadSynthkit(bool mode_, uint8_t kitNum_);
    SdResult sd_saveSynthkit(bool mode_, uint8_t kitNum_);
    SdResult sd_clearSynthkit(bool mode_, uint8_t kitNum_);
    SdResult sd_checkWavetablesInUse();
    FRESULT sd_createDirectory(const char *path);
    FRESULT sd_deleteDirectory(const char *path);

    static inline int compareWords(const void *str1, const void *str2) {
        return strcmp(*(char **)str1, *(char **)str2);
    }

    static inline void sortWords(char *words[], int count) {
        qsort(words, count, sizeof(words[0]), compareWords);
    }

    /* Sdram functions -------------------------------------------------------*/

    void sdram_write16BitAudio(uint32_t ramAddress_, int16_t data_);
    int16_t sdram_read16BitAudio(uint32_t ramAddress_);
    void sdram_write24BitAudio(uint32_t ramAddress_, int32_t data_);
    int32_t sdram_read24BitAudio(uint32_t ramAddress_);
    void sdram_fadeOut24BitAudio(uint32_t ramAddress_, uint32_t sampleSize_, uint16_t fadeOutSize_);

    /* Lcd functions ---------------------------------------------------------*/

    void lcd_initialize();
    void lcd_test();
    void lcd_update();
    void lcd_drawLogo();
    void lcd_drawPage();
    // alert functions
    void lcd_drawAlert();
    void lcd_clearAlert();
    // limit functions
    void lcd_drawLimitAlert();
    // sd functions
    void lcd_drawSdDataIntro();
    void lcd_drawSdData();
    void lcd_clearSdData();
    void lcd_drawSdAlert(SdResult result_);
    void lcd_clearSdAlert();
    void lcd_drawInitSdAlert(SdResult result_);
    void lcd_clearInitSdAlert();
    // menu functions
    void lcd_drawMenuIcon(Menu menu_);
    void lcd_drawFileBox(uint8_t menuTab_, FileStatus status_);
    void lcd_clearFileBox(uint8_t menuTab);
    void lcd_transitionMenu();
    void lcd_transitionSelect();
    void lcd_setMenuHeaderState(RGB16Color color_);
    void lcd_setMenuDataState(RGB16Color color_);
    void lcd_setMenuNumState(RGB16Color color_);
    void lcd_setMenuSignState(RGB16Color color_, LcdFont font_);
    void lcd_setMainMenuHeaderState();
    void lcd_setSongDataState();
    // main menu functions
    void lcd_drawMainMenu();
    void lcd_drawMain_TempoData();
    void lcd_drawMain_MeasureData();
    void lcd_drawMain_BarData();
    void lcd_drawMain_QuantizeData();
    void lcd_drawMain_Filter0Data();
    void lcd_drawMain_Filter1Data();
    void lcd_drawMain_Effect0Data();
    void lcd_drawMain_Effect1Data();
    // file menu functions
    void lcd_drawFileMenu();
    void lcd_drawFile_NewData();
    void lcd_drawFile_LoadData();
    void lcd_drawFile_SaveData();
    void lcd_drawFile_ClearData();
    // synthkit menu functions
    void lcd_drawSynthkitMenu();
    void lcd_drawSynthkit_NewData();
    void lcd_drawSynthkit_LoadData();
    void lcd_drawSynthkit_SaveData();
    void lcd_drawSynthkit_ClearData();
    // system menu functions
    void lcd_drawSystemMenu();
    void lcd_drawSystem_VolumeData();
    void lcd_drawSystem_PanData();
    void lcd_drawSystem_LimiterData();
    void lcd_drawSystem_MidiInData();
    void lcd_drawSystem_MidiOutData();
    void lcd_drawSystem_SyncInData();
    void lcd_drawSystem_SyncOutData();
    // rhythm menu functions
    void lcd_drawRhythmMenu();
    void lcd_drawRhythm_TempoData();
    void lcd_drawRhythm_MeasureData();
    void lcd_drawRhythm_BarData();
    void lcd_drawRhythm_QuantizeData();
    // metronome menu functions
    void lcd_drawMetroMenu();
    void lcd_drawMetro_ActiveData();
    void lcd_drawMetro_PrecountData();
    void lcd_drawMetro_SampleData();
    void lcd_drawMetro_VolumeData();
    // eq menu functions
    void lcd_drawEqMenu();
    void lcd_drawEq_ActiveData();
    void lcd_drawEq_LowShelfData();
    void lcd_drawEq_HighShelfData();
    void lcd_drawEq_PeakData(uint8_t peakNum_);
    // osc functions
    void lcd_drawOsc0Menu(Osc &osc_);
    void lcd_drawOsc1Menu(Osc &osc_);
    void lcd_drawOscLfoMenu(Osc &osc_, Lfo &lfo_);
    void lcd_drawOsc_ActiveData(Osc &osc_);
    void lcd_drawOsc_WavetableData(Osc &osc_);
    void lcd_drawOsc_LevelData(Osc &osc_);
    void lcd_drawOsc_TuneData(Osc &osc_);
    void lcd_drawOsc_PhaseData(Osc &osc_);
    void lcd_drawOsc_NormalizeData(Osc &osc_);
    void lcd_drawOsc_StartData(Osc &osc_);
    void lcd_drawOsc_EndData(Osc &osc_);
    void lcd_drawOsc_XFlipData(Osc &osc_);
    void lcd_drawOsc_YFlipData(Osc &osc_);
    void lcd_drawOscLfo_ActiveData(Osc &osc_, Lfo &lfo_);
    void lcd_drawOscLfo_TypeData(Osc &osc_, Lfo &lfo_);
    void lcd_drawOscLfo_TargetData(Osc &osc_, Lfo &lfo_);
    void lcd_drawOscLfo_RateData(Osc &osc_, Lfo &lfo_);
    void lcd_drawOscLfo_DepthData(Osc &osc_, Lfo &lfo_);
    void lcd_drawOscLfo_LoopData(Osc &osc_, Lfo &lfo_);
    // filter menu functions
    void lcd_drawFilterMenu(uint8_t filterNum_);
    void lcd_drawFilter_ActiveData(uint8_t filterNum_);
    void lcd_drawFilter_TypeData(uint8_t filterNum_);
    void lcd_drawFilter_FreqData(uint8_t filterNum_);
    void lcd_drawFilter_ResData(uint8_t filterNum_);
    void lcd_drawFilter_SlopeData(uint8_t filterNum_);
    void lcd_drawFilter_DryData(uint8_t filterNum_);
    void lcd_drawFilter_WetData(uint8_t filterNum_);
    // envelope functions
    void lcd_drawEnvelopeMenu();
    void lcd_drawEnvelope_ActiveData();
    void lcd_drawEnvelope_TypeData();
    void lcd_drawEnvelope_CurveData();
    void lcd_drawEnvelope_AttackTimeData();
    void lcd_drawEnvelope_DecayTimeData();
    void lcd_drawEnvelope_SustainLevelData();
    void lcd_drawEnvelope_ReleaseTimeData();
    // effect menu functions
    void lcd_drawEffectMenu(uint8_t effectNum_);
    void lcd_drawEffect_ActiveData(uint8_t effectNum_);
    void lcd_drawEffect_TypeData(uint8_t effectNum_);
    void lcd_drawEffect_AData(uint8_t effectNum_);
    void lcd_drawEffect_BData(uint8_t effectNum_);
    void lcd_drawEffect_CData(uint8_t effectNum_);
    void lcd_drawEffect_DData(uint8_t effectNum_);
    void lcd_drawEffect_EData(uint8_t effectNum_);
    // reverb menu functions
    void lcd_drawReverbMenu();
    void lcd_drawReverb_ActiveData();
    void lcd_drawReverb_SizeData();
    void lcd_drawReverb_DecayData();
    void lcd_drawReverb_PreDelayData();
    void lcd_drawReverb_SurroundData();
    void lcd_drawReverb_DryData();
    void lcd_drawReverb_WetData();
    // key menu functions
    void lcd_drawKeyMenu();
    void lcd_drawKey_NoteData();
    void lcd_drawKey_ArpegData();
    void lcd_drawKey_RateData();
    void lcd_drawKey_OscData();
    void lcd_drawKey_ChordData();
    void lcd_drawKey_OrderData();
    void lcd_drawKey_OctaveData();
    // song menu functions
    void lcd_drawSongMenu();
    void lcd_drawSong_BeatData();
    void lcd_drawSong_BeatGraph();
    // bank functions
    void lcd_drawBank(uint8_t bankNum_, bool mode_);
    void lcd_drawBankShift();
    // octave functions
    void lcd_drawOctave();
    // transition functions
    void lcd_drawTransition();
    // play functions
    void lcd_drawPlay();
    void lcd_drawIcon();
    void lcd_drawText();
    void lcd_drawCountDown();
    void lcd_clearCountDown();
    void lcd_restartPlay();
    void lcd_resetPlay();
    void lcd_redrawPlay();
    void lcd_cleanEndPlay();
    void lcd_invertPlayColor();
    void lcd_resetPlayColor();
    // song functions
    void lcd_calculateSongX();
    void lcd_drawMeasureBar();
    void lcd_drawStartBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_);
    void lcd_drawEndBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_);
    void lcd_drawBeat(uint8_t bankNum_, uint8_t beatNum_, bool selected_);
    void lcd_clearBeat(uint8_t bankNum_, uint8_t beatNum_);
    void lcd_drawSong(uint8_t bankNum_);
    void lcd_clearSong();
    void lcd_clearBeatInterval(uint8_t bankNum_, uint8_t beatNum_, uint16_t startInterval_, uint16_t endInterval_);
    // info functions
    // info song
    void lcd_drawInfo_Song_Select(bool selected);
    // info key
    void lcd_drawInfo_Key_Select(bool selected);
    void lcd_drawInfo_Key_NoteData();
    void lcd_drawInfo_Key_ArpegData();
    void lcd_drawInfo_Key_RateData();
    void lcd_drawInfo_Key_OscData();
    void lcd_drawInfo_Key_ChordData();
    void lcd_drawInfo_Key_OrderData();
    void lcd_drawInfo_Key_OctaveData();
    void lcd_drawInfo_Key_PatternData();
    // info osc
    void lcd_drawInfo_Osc_Select(Osc &osc_, uint8_t selected);
    void lcd_drawInfo_OscLfo_Select(Osc &osc_, Lfo &lfo_, bool selected);
    void lcd_drawInfo_Osc_ActiveData(Osc &osc_);
    void lcd_drawInfo_Osc_WavetableData(Osc &osc_);
    void lcd_drawInfo_Osc_LevelData(Osc &osc_);
    void lcd_drawInfo_Osc_TuneData(Osc &osc_);
    void lcd_drawInfo_Osc_PhaseData(Osc &osc_);
    void lcd_drawInfo_Osc_GraphData(Osc &osc_);
    void lcd_drawInfo_OscLfo_ActiveData(Osc &osc_, Lfo &lfo_);
    void lcd_drawInfo_OscLfo_TypeData(Osc &osc_, Lfo &lfo_);
    void lcd_drawInfo_OscLfo_TargetData(Osc &osc_, Lfo &lfo_);
    void lcd_drawInfo_OscLfo_RateData(Osc &osc_, Lfo &lfo_);
    void lcd_drawInfo_OscLfo_GraphData(Osc &osc_, Lfo &lfo_);
    // info filter
    void lcd_drawInfo_Filter_Select(uint8_t filterNum_, bool selected);
    void lcd_drawInfo_Filter_ActiveData(uint8_t filterNum_);
    void lcd_drawInfo_Filter_TypeData(uint8_t filterNum_);
    void lcd_drawInfo_Filter_FreqData(uint8_t filterNum_);
    void lcd_drawInfo_Filter_ResData(uint8_t filterNum_);
    // info envelope
    void lcd_drawInfo_Envelope_Select(bool selected);
    void lcd_drawInfo_Envelope_ActiveData();
    void lcd_drawInfo_Envelope_TypeData();
    void lcd_drawInfo_Envelope_CurveData();
    void lcd_drawInfo_Envelope_AttackTimeData();
    void lcd_drawInfo_Envelope_DecayTimeData();
    void lcd_drawInfo_Envelope_SustainLevelData();
    void lcd_drawInfo_Envelope_ReleaseTimeData();

    /* Main functions --------------------------------------------------------*/

    void main_select();

    /* File functions --------------------------------------------------------*/

    void file_select();

    void file_menuRight();
    void file_menuLeft();
    void file_menuUp();
    void file_menuDown();

    void file_newSelect();
    void file_newAction();
    void file_loadSelect();
    void file_loadAction();
    void file_saveSelect();
    void file_saveAction();
    void file_clearSelect();
    void file_clearAction();

    /* Synthkit functions ----------------------------------------------------*/

    void synthkit_select();

    void synthkit_menuRight();
    void synthkit_menuLeft();
    void synthkit_menuUp();
    void synthkit_menuDown();

    void synthkit_newSelect();
    void synthkit_newAction();
    void synthkit_loadSelect();
    void synthkit_loadAction();
    void synthkit_saveSelect();
    void synthkit_saveAction();
    void synthkit_clearSelect();
    void synthkit_clearAction();

    /* System functions ------------------------------------------------------*/

    void system_select();
    void system_reset();

    void system_menuRight();
    void system_menuLeft();
    void system_menuUp();
    void system_menuDown();

    void system_setVolume(uint8_t volume_);
    void system_setPan(uint8_t pan_);
    void system_setLimiter(bool mode_);
    void system_setMidiIn(uint8_t mode_);
    void system_setMidiOut(uint8_t mode_);
    void system_setSyncIn(uint8_t mode_);
    void system_setSyncOut(uint8_t mode_);

    void system_volumeTransition(float volumeFloat_);
    void system_panTransition(float volumeLeftFloat_, float volumeRightFloat_);
    void system_calculateVolumeTransition();
    void system_calculatePanTransition();

    /* Rhythm functions ------------------------------------------------------*/

    void preMenuSongClear();

    void rhythm_select();
    void rhythm_reset();

    void rhythm_menuRight();
    void rhythm_menuLeft();
    void rhythm_menuUp();
    void rhythm_menuDown();

    void rhythm_setTempo(uint8_t tempo_);
    void rhythm_setMeasure(uint8_t measure_);
    void rhythm_setBar(uint8_t bar_);
    void rhythm_setQuantize(uint8_t quantize_);

    /* Metronome functions ---------------------------------------------------*/

    void metro_select();
    void metro_reset();

    void metro_menuRight();
    void metro_menuLeft();
    void metro_menuUp();
    void metro_menuDown();

    void metro_setActive(bool active_);
    void metro_setPrecount(bool precount_);
    void metro_setSample(uint8_t sample_);
    void metro_setVolume(uint8_t volume_);

    void metro_volumeTransition(float volumeFloat);
    void metro_calculateVolumeTransition();

    /* Eq functions ----------------------------------------------------------*/

    void eq_select();
    void eq_reset();

    void eq_menuRight();
    void eq_menuLeft();
    void eq_menuUp();
    void eq_menuDown();

    void eq_setActive(bool active_);
    void eq_setFreqLowShelf(uint8_t freq_);
    void eq_setGainLowShelf(uint8_t gain_);
    void eq_setFreqHighShelf(uint8_t freq_);
    void eq_setGainHighShelf(uint8_t gain_);
    void eq_setFreqPeak(uint8_t peakNum_, uint8_t freq_);
    void eq_setGainPeak(uint8_t peakNum_, uint8_t gain_);
    void eq_setQPeak(uint8_t peakNum_, uint8_t q_);

    void eq_genTransition(EqTransitionMode mode_, bool activeActive_, bool targetActive_);
    void eq_calculateActiveTransition();

    /* Osc functions ---------------------------------------------------------*/

    void osc_select(Osc &osc_);
    void osc_reset(Osc &osc_);
    void osc_lfo_reset(Osc &osc_, Lfo &lfo_);

    void osc_menuRight(Osc &osc_);
    void osc_menuLeft(Osc &osc_);
    void osc_menuUp(Osc &osc_);
    void osc_menuDown(Osc &osc_);

    void osc_setActive(Osc &osc_, bool active_);
    void osc_setWavetableSelected(Osc &osc_, int16_t wavetable_);
    void osc_setWavetableLoaded(Osc &osc_, bool view_);
    void osc_setLevel(Osc &osc_, uint8_t level_);
    void osc_setTune(Osc &osc_, uint8_t tune_);
    void osc_setPhase(Osc &osc_, uint8_t phase_);
    void osc_setNormalize(Osc &osc_, bool norm_);
    void osc_setStart(Osc &osc_, uint16_t start_);
    void osc_setEnd(Osc &osc_, uint16_t end_);
    void osc_setXFlip(Osc &osc_, bool xFlip_);
    void osc_setYFlip(Osc &osc_, bool yFlip_);

    void osc_lfo_setActive(Osc &osc_, Lfo &lfo_, uint8_t active_);
    void osc_lfo_setType(Osc &osc_, Lfo &lfo_, uint8_t type_);
    void osc_lfo_setTarget(Osc &osc_, Lfo &lfo_, uint8_t target_);
    void osc_lfo_setRate(Osc &osc_, Lfo &lfo_, uint8_t rate_);
    void osc_lfo_setDepth(Osc &osc_, Lfo &lfo_, uint8_t depth_);
    void osc_lfo_setLoop(Osc &osc_, Lfo &lfo_, bool loop_);

    /* Filter functions ------------------------------------------------------*/

    void filter_select();
    void filter_reset(uint8_t filterNum_);

    void filter_menuRight();
    void filter_menuLeft();
    void filter_menuUp();
    void filter_menuDown();

    void filter_setActive(uint8_t filterNum_, bool active_);
    void filter_setType(uint8_t filterNum_, uint8_t type_);
    void filter_setFreq(uint8_t filterNum_, uint8_t freq_);
    void filter_setRes(uint8_t filterNum_, uint8_t res_);
    void filter_setSlope(uint8_t filterNum_, uint8_t slope_);
    void filter_setDry(uint8_t filterNum_, uint8_t dry_);
    void filter_setWet(uint8_t filterNum_, uint8_t wet_);

    void filter_genTransition(uint8_t filterNum_, FilterTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_);
    void filter_mixTransition(uint8_t filterNum_, float dryFloat, float wetFloat);
    void filter_calculateGenTransition(uint8_t filterNum_);
    void filter_calculateMixTransition(uint8_t filterNum_);

    /* Envelope functions ----------------------------------------------------*/

    void envelope_select();
    void envelope_reset();

    void envelope_menuRight();
    void envelope_menuLeft();
    void envelope_menuUp();
    void envelope_menuDown();

    void envelope_setActive(bool active_);
    void envelope_setType(uint8_t type_);
    void envelope_setCurve(uint8_t curve_);
    void envelope_setAttackTime(uint8_t time_);
    void envelope_setDecayTime(uint8_t time_);
    void envelope_setSustainLevel(uint8_t level_);
    void envelope_setReleaseTime(uint8_t time_);

    /* Effect functions ------------------------------------------------------*/

    void effect_select();
    void effect_reset(uint8_t effectNum_);

    void effect_menuRight();
    void effect_menuLeft();
    void effect_menuUp();
    void effect_menuDown();

    void effect_setActive(uint8_t effectNum_, bool active_);
    void effect_setType(uint8_t effectNum_, uint8_t type_);
    void effect_setAData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t aData_);
    void effect_setBData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t bData_);
    void effect_setCData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t cData_);
    void effect_setDData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t dData_);
    void effect_setEData(uint8_t effectNum_, uint8_t subEffectNum_, uint8_t eData_);

    void effect_genTransition(uint8_t effectNum_, EffectTransitionMode mode_, bool activeActive_, bool targetActive_, uint8_t activeType_, uint8_t targetType_);
    void effect_mixTransition(uint8_t effectNum_, float dryFloat_, float wetFloat_);
    void effect_calculateGenTransition(uint8_t effectNum_);
    void effect_calculateMixTransition(uint8_t effectNum_);

    void effect_cleanMemory(uint8_t effectNum_, uint8_t type_);

    /* Reverb functions ------------------------------------------------------*/

    void reverb_select();
    void reverb_reset();

    void reverb_menuRight();
    void reverb_menuLeft();
    void reverb_menuUp();
    void reverb_menuDown();

    void reverb_setActive(bool active_);
    void reverb_setSize(uint8_t size_);
    void reverb_setDecay(uint8_t decay_);
    void reverb_setPreDelay(uint8_t preDelay_);
    void reverb_setSurround(uint8_t surround_);
    void reverb_setDry(uint8_t dry_);
    void reverb_setWet(uint8_t wet_);

    void reverb_genTransition(ReverbTransitionMode mode_, bool activeActive_, bool targetActive_);
    void reverb_mixTransition(float dryFloat, float wetFloat);
    void reverb_calculateGenTransition();
    void reverb_calculateMixTransition();

    /* Key functions ---------------------------------------------------------*/

    void key_select();
    void key_reset();

    void key_menuRight();
    void key_menuLeft();
    void key_menuUp();
    void key_menuDown();

    void key_setNote(uint8_t note_);
    void key_setArpeg(bool arpeg_);
    void key_setRate(uint8_t rate_);
    void key_setOsc(uint8_t osc_);
    void key_setChord(uint8_t chord_);
    void key_setOrder(uint8_t order_);
    void key_setOctave(uint8_t octave_);
    void key_calculateArpeg();

    /* Song functions --------------------------------------------------------*/

    void song_select();
    void song_reset();

    void song_menuRight();
    void song_menuLeft();
    void song_menuUp();
    void song_menuDown();

    void song_startRecordBeat(uint8_t bankNum_, uint16_t interval_, uint8_t octaveNum_, uint8_t noteNum_);
    void song_endRecordBeat(uint8_t bankNum_, uint16_t interval_);
    void song_setBeat(uint8_t bankNum_, uint16_t startInterval_, uint16_t endInterval_, uint8_t octaveNum_, uint8_t noteNum_);
    void song_setBeatNoteOctave(uint8_t note_, uint8_t octave_);
    void song_generateBeat(uint8_t type);
    void song_resetSelectedBeat();
    void song_resetBeat(uint8_t bankNum_, uint8_t beatNum_);
    void song_resetBeats(uint8_t bankNum_, uint16_t startInterval_);
    void song_resetAllBeats(uint8_t bankNum_);
    void song_quantizeActiveBeats(uint8_t bankNum_);
    void song_arrangeActiveBeats(uint8_t bankNum_, bool duplicate_, bool collect_, bool sort_);
    void song_calculateLastActiveBeatNum(uint8_t bankNum_);
    void song_calculateNextBeatNum(uint8_t bankNum_, uint16_t playInterval_);

    /* Bank functions --------------------------------------------------------*/

    void bank_select(uint8_t bankNum_);
    void bank_trigger(uint8_t bankNum_);

    /* Note functions --------------------------------------------------------*/

    void pressNote(uint8_t octaveNum_, uint8_t note_);
    void releaseNote();

    /* Octave functions ------------------------------------------------------*/

    void octaveUp();
    void octaveDown();

    void setOctave(uint8_t octave_);

    /* Play functions --------------------------------------------------------*/

    void record();
    void play();
    void stop();
    void reset();

    void triggerStop();
    void triggerReset();

    /* PlayData functions ----------------------------------------------------*/

    void startBeatPlayData(uint8_t beatNum_);
    void releaseBeatPlayData(uint8_t beatNum_);
    void endBeatPlayData(uint8_t beatNum_);
    void calculateBeatPlayData(uint8_t beatNum_);
    bool checkSdramReadActive();

    /* Interrupt functions ---------------------------------------------------*/

    void interruptPlay();

    void interruptAudioMetronome();
    void interruptAudioSong();
    void interruptAudioLpf();
    void interruptAudioEq();
    void interruptAudioFilter();
    void interruptAudioEffect();
    void interruptAudioReverb();
    void interruptAudioSend();

    int32_t processAudioFilter(uint8_t filterNum_, int32_t audio_);
    int32_t processAudioEffect(uint8_t effectNum_, int32_t audio_);

    void interruptTransition();

    void interruptLeftButtonTrigger();
    void interruptRightButtonTrigger();
    void interruptBeatButtonTrigger();
    void interruptLeftButtonRead();
    void interruptRightButtonRead();
    void interruptBeatButtonRead();
    void interruptUpDownButtonRead();
    void interruptLongButtonRead();
    void interruptPowerButtonRead();

    void interruptText();
    void interruptSd();
    void interruptBeatSync();
    void interruptLimitAlert();

    /* Debug functions -------------------------------------------------------*/

    void check(int32_t num_, uint8_t line_) {
        lcd.setAlignment(LEFT);
        lcd.setFont(FONT_05x07);
        lcd.setForeColor(WHITE);
        lcd.setBackColor(BLACK);
        lcd.drawNumber(num_, 8, 30, 30 + (line_ * 10));
    }
};

#endif
