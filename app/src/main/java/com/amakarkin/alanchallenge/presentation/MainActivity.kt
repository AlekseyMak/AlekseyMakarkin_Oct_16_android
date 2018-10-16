package com.amakarkin.alanchallenge.presentation

import android.Manifest
import android.animation.Animator
import android.animation.ValueAnimator
import android.content.pm.PackageManager
import android.os.Bundle
import android.support.v4.app.ActivityCompat
import android.support.v4.content.ContextCompat
import android.view.View
import android.view.animation.AccelerateDecelerateInterpolator
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
                    101)
        }

        // Example of a call to a native method
        sample_text.text = stringFromJNI()

        start_record.setOnClickListener {
            energy_meter.visibility = View.VISIBLE
            presenter.startRecording()
        }

        stop_record.setOnClickListener {
            presenter.stopRecord()
            energy_meter.visibility = View.GONE
        }
    }

    private var prevLevel = 0

    override fun onEnergyLevel(level: Int) {
        runOnUiThread {
            val energyAnimator = ValueAnimator.ofInt(prevLevel, level)
            energyAnimator.duration = 20
            energyAnimator.addUpdateListener {
                energy_meter.progress = it.animatedValue as Int
            }
            energyAnimator.start()
            prevLevel = level
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
