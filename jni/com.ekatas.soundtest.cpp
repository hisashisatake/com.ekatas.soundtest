#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define LOCAL_LOG
#ifdef	LOCAL_LOG
#include <android/log.h>
#endif

#if 0
// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#endif

// WAVE DATA
typedef struct
{
  int fs; /* 標本化周波数 */
  int bits; /* 量子化精度 */
  int length; /* 音データの長さ */
  double *s; /* 音データ */
} MONO_PCM;

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
//static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;


// synthesized sawtooth clip
#define SAWTOOTH_FRAMES 44100 * 4
static short sawtoothBuffer[SAWTOOTH_FRAMES];

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    assert(bq == bqPlayerBufferQueue);
    assert(NULL == context);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (--nextCount > 0 && NULL != nextBuffer && 0 != nextSize) {
        SLresult result;
        // enqueue another buffer
        result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        assert(SL_RESULT_SUCCESS == result);
    }
}

// create the engine and output mix objects
void Java_com_ekatas_soundtest_MainActivity_createEngine(JNIEnv* env, jclass clazz)
{
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

    // create output mix, with environmental reverb specified as a non-required interface
    //const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLInterfaceID ids[1] = {SL_IID_VOLUME};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

#if 0
    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
            &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
    }
    // ignore unsuccessful result codes for environmental reverb, as it is optional for this example
#endif

}

// create buffer queue audio player
void Java_com_ekatas_soundtest_MainActivity_createBufferQueueAudioPlayer(JNIEnv* env,
        jclass clazz)
{
    SLresult result;

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
            &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // get the effect send interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
            &bqPlayerEffectSend);
    assert(SL_RESULT_SUCCESS == result);

   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
}

void ADSR(double e[], int A, int D, double S, int R, int gate, int duration)
{
  int n;

  if (A != 0)
  {
    for (n = 0; n < A; n++)
    {
      e[n] = 1.0 - exp(-5.0 * n / A);
    }
  }

  if (D != 0)
  {
    for (n = A; n < gate; n++)
    {
      e[n] = S + (1 - S) * exp(-5.0 * (n - A) / D);
    }
  }
  else
  {
    for (n = A; n < gate; n++)
    {
      e[n] = S;
    }
  }

  if (R != 0)
  {
    for (n = gate; n < duration; n++)
    {
      e[n] = e[gate - 1] * exp(-5.0 * (n - gate + 1) / R);
    }
  }
}

/*
 * Class:     com_ekatas_soundtest_MainActivity
 * Method:    outputSound
 * Signature: ()V
 */
JNIEXPORT jboolean JNICALL Java_com_ekatas_soundtest_MainActivity_outputSound
  (JNIEnv *env, jobject thiz, jint which, jint count)
{
    MONO_PCM pcm;
    int n, A, D, R, gate, duration;
    double *ac, fc, *am, fm, ratio, gain, S;
    double num;

    pcm.fs = 44100; /* 標本化周波数 */
    pcm.bits = 16; /* 量子化精度 */
    pcm.length = pcm.fs * 4; /* 音データの長さ */
    pcm.s = (double*)calloc(pcm.length, sizeof(double)); /* 音データ */

    ac = (double*)calloc(pcm.length, sizeof(double));
    am = (double*)calloc(pcm.length, sizeof(double));

    /* キャリア振幅 */
    gate = pcm.fs * 4;
    duration = pcm.fs * 4;
    A = 0;
    D = pcm.fs * 4;
    S = 0.0;
    R = pcm.fs * 4;
    ADSR(ac, A, D, S, R, gate, duration);

    fc = 440.0; /* キャリア周波数 */

    /* モジュレータ振幅 */
    gate = pcm.fs * 4;
    duration = pcm.fs * 4;
    A = 0;
    D = pcm.fs * 2;
    S = 0.0;
    R = pcm.fs * 2;
    ADSR(am, A, D, S, R, gate, duration);

    ratio = 3.5;
    fm = fc * ratio; /* モジュレータ周波数 */

    /* FM音源 */
//    for (n = 0; n < pcm.length; n++)
    for (n = 0; n < SAWTOOTH_FRAMES; ++n)
    {
    	num = ac[n] * sin(2.0 * M_PI * fc * n / pcm.fs + am[n] * sin(2.0 * M_PI * fm * n / pcm.fs));
    	sawtoothBuffer[n] = num * 10000;
    	/*
      	pcm.s[n] = ac[n] * sin(2.0 * M_PI * fc * n / pcm.fs
            	 + am[n] * sin(2.0 * M_PI * fm * n / pcm.fs));
        */

#if 0
    	//ログを出力
    	__android_log_print(ANDROID_LOG_DEBUG,"soundtest","sawtooth:%f",num);
    	__android_log_print(ANDROID_LOG_DEBUG,"soundtest","sawtooth:%f",sawtoothBuffer[n]);
#endif
	}

    free(pcm.s);
    free(ac);
    free(am);

    switch (which) {
    case 0:     // CLIP_NONE
        nextBuffer = (short *) NULL;
        nextSize = 0;
        break;
    case 1:     // CLIP_SAWTOOTH
        nextBuffer = sawtoothBuffer;
        nextSize = sizeof(sawtoothBuffer);
        break;
    default:
        nextBuffer = NULL;
        nextSize = 0;
        break;
    }

    nextCount = count;
    if (nextSize > 0) {
    	// here we only enqueue one buffer because it is a long clip,
    	// but for streaming playback we would typically enqueue at least 2 buffers to start
    	SLresult result;
    	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);
    	if (SL_RESULT_SUCCESS != result) {
    		return JNI_FALSE;
    	}
    }

    return JNI_TRUE;
}

void Java_com_ekatas_soundtest_MainActivity_shutdown(JNIEnv* env, jclass clazz)
{

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerEffectSend = NULL;
        bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }

#if 0
    // destroy file descriptor audio player object, and invalidate all associated interfaces
    if (fdPlayerObject != NULL) {
        (*fdPlayerObject)->Destroy(fdPlayerObject);
        fdPlayerObject = NULL;
        fdPlayerPlay = NULL;
        fdPlayerSeek = NULL;
        fdPlayerMuteSolo = NULL;
        fdPlayerVolume = NULL;
    }

    // destroy URI audio player object, and invalidate all associated interfaces
    if (uriPlayerObject != NULL) {
        (*uriPlayerObject)->Destroy(uriPlayerObject);
        uriPlayerObject = NULL;
        uriPlayerPlay = NULL;
        uriPlayerSeek = NULL;
        uriPlayerMuteSolo = NULL;
        uriPlayerVolume = NULL;
    }

    // destroy audio recorder object, and invalidate all associated interfaces
    if (recorderObject != NULL) {
        (*recorderObject)->Destroy(recorderObject);
        recorderObject = NULL;
        recorderRecord = NULL;
        recorderBufferQueue = NULL;
    }
#endif

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        //outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

}

/*
 * Class:     com_ekatas_soundtest_MainActivity
 * Method:    stringFromJNI
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_ekatas_soundtest_MainActivity_stringFromJNI
  (JNIEnv *env, jobject thiz)
{
	return env->NewStringUTF("");
}

#ifdef __cplusplus
}
#endif
