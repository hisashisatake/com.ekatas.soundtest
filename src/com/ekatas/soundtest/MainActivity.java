package com.ekatas.soundtest;

import android.os.Bundle;
import android.widget.Button;
import android.app.Activity;
import android.view.Menu;
import android.view.View;

public class MainActivity extends Activity {
	static {
		System.loadLibrary("com.ekatas.soundtest");
	}
    /** Native methods, implemented in jni folder */
    public static native void createEngine();
    public static native void createBufferQueueAudioPlayer();
    /*
    public static native boolean createAssetAudioPlayer(AssetManager assetManager, String filename);
    // true == PLAYING, false == PAUSED
    public static native void setPlayingAssetAudioPlayer(boolean isPlaying);
    public static native boolean createUriAudioPlayer(String uri);
    public static native void setPlayingUriAudioPlayer(boolean isPlaying);
    public static native void setLoopingUriAudioPlayer(boolean isLooping);
    public static native void setChannelMuteUriAudioPlayer(int chan, boolean mute);
    public static native void setChannelSoloUriAudioPlayer(int chan, boolean solo);
    public static native int getNumChannelsUriAudioPlayer();
    public static native void setVolumeUriAudioPlayer(int millibel);
    public static native void setMuteUriAudioPlayer(boolean mute);
    public static native void enableStereoPositionUriAudioPlayer(boolean enable);
    public static native void setStereoPositionUriAudioPlayer(int permille);
    */
    public static native boolean selectClip(int which, int count);
    /*
    public static native boolean enableReverb(boolean enabled);
    public static native boolean createAudioRecorder();
    public static native void startRecording();
    */
    public static native void shutdown();

    public native void outputSound(int which, int count);
	public native String stringFromJNI();

    static final int CLIP_NONE = 0;
    static final int CLIP_SAWTOOTH = 1;

    private Button buttonOutput;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

        // initialize native audio system
        createEngine();
        createBufferQueueAudioPlayer();

		buttonOutput = (Button)findViewById(R.id.buttonOutput);
		buttonOutput.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				outputSound(CLIP_SAWTOOTH, 0);
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onPause()
    {
        // turn off all audio
        outputSound(CLIP_NONE, 0);
        super.onPause();
    }
    
    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
        shutdown();
        super.onDestroy();
    }
}
