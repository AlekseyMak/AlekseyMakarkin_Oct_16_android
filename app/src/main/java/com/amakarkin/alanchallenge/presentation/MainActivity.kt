package com.amakarkin.alanchallenge.presentation

import android.Manifest
import android.animation.ValueAnimator
import android.content.pm.PackageManager
import android.os.Bundle
import android.support.v4.app.ActivityCompat
import android.support.v4.content.ContextCompat
import android.util.Log
import android.view.View
import android.widget.SeekBar
import android.widget.Toast
import com.amakarkin.alanchallenge.R
import com.amakarkin.alanchallenge.base.BaseMvpActivity
import com.arellomobile.mvp.presenter.InjectPresenter
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : BaseMvpActivity(), MainView {

    @InjectPresenter
    lateinit var presenter: MainPresenter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    arrayOf(Manifest.permission.RECORD_AUDIO),
                    PERMISSION_REQUEST_CODE)
        }

        tempo.incrementProgressBy(1)
        tempo.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                //
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                //
            }

            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                presenter.setPlayBackSpeed(progress)
            }

        })

        start_record.setOnClickListener {
            start_playback.isEnabled = false
            stop_playback.isEnabled = false
            energy_meter.visibility = View.VISIBLE
            presenter.startRecording(this)
        }

        stop_record.setOnClickListener {
            start_playback.isEnabled = true
            stop_playback.isEnabled = true
            presenter.stopRecord()
            energy_meter.visibility = View.GONE
        }

        start_playback.setOnClickListener {
            tempo.progress = 3
            tempo.visibility = View.VISIBLE
            start_record.isEnabled = false
            stop_record.isEnabled = false
            energy_meter.visibility = View.VISIBLE
            presenter.play(this)
        }

        stop_playback.setOnClickListener {
            tempo.visibility = View.GONE
            start_record.isEnabled = true
            stop_record.isEnabled = true
            presenter.stopPlayback()
            energy_meter.visibility = View.GONE
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int,
                                            permissions: Array<String>, grantResults: IntArray) {
        when (requestCode) {
            PERMISSION_REQUEST_CODE -> {
                // If request is cancelled, the result arrays are empty.
                if ((grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED)) {
                    // permission was granted, yay! Do the
                    // contacts-related task you need to do.
                } else {
                    onError("You need to give permissions!")
                    finish()
                }
                return
            }

            // Add other 'when' lines to check for other
            // permissions this app might request.
            else -> {
                // Ignore all other requests.
            }
        }
    }
    private var prevLevel = 0

    override fun onEnergyLevel(level: Int) {
        val energyAnimator = ValueAnimator.ofInt(prevLevel, level)
        energyAnimator.duration = 20
        energyAnimator.addUpdateListener {
            energy_meter.progress = it.animatedValue as Int
        }
        energyAnimator.start()
        prevLevel = level
    }


    override fun onError(error: String) {
        runOnUiThread {
            Toast.makeText(this, error, Toast.LENGTH_SHORT).show()
        }
    }

    override fun onResume() {
        super.onResume()
        getVersion()
    }

    public external fun getVersion()

    companion object {

        private val PERMISSION_REQUEST_CODE = 101

        init {
            System.loadLibrary("native-lib")
//            System.loadLibrary("avcodec")
        }
    }
}
