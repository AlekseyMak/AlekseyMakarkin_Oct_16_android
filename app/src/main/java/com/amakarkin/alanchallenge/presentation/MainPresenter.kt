package com.amakarkin.alanchallenge.presentation

import android.util.Log
import com.amakarkin.alanchallenge.domain.Recorder
import com.arellomobile.mvp.InjectViewState
import com.arellomobile.mvp.MvpPresenter
import com.arellomobile.mvp.MvpView

interface MainView : MvpView {

}

@InjectViewState
class MainPresenter : MvpPresenter<MainView>() {

    fun startRecording() {
        val recorder = Recorder()
        recorder.recordAudio()
                .subscribe{
                    Log.e("Presenter", it.toString())
                }
    }
}