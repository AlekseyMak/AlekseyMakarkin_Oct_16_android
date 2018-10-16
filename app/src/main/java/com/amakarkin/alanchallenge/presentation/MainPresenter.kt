package com.amakarkin.alanchallenge.presentation

import android.content.Context
import android.util.Log
import com.amakarkin.alanchallenge.domain.Recorder
import com.arellomobile.mvp.InjectViewState
import com.arellomobile.mvp.MvpPresenter
import com.arellomobile.mvp.MvpView
import com.arellomobile.mvp.viewstate.strategy.SkipStrategy
import com.arellomobile.mvp.viewstate.strategy.StateStrategyType
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.Disposable

interface MainView : MvpView {
    @StateStrategyType(SkipStrategy::class)
    fun onEnergyLevel(level: Int)
}

@InjectViewState
class MainPresenter : MvpPresenter<MainView>() {

    private var recorder: Recorder? = null

    private var disposable: Disposable? = null

    fun startRecording(context: Context) {
        recorder = Recorder(context)

        disposable = recorder?.recordAudio()
                ?.subscribeOn(AndroidSchedulers.mainThread())
                ?.subscribe{
                    Log.e("Presenter", it.toString())
                    viewState.onEnergyLevel(it)
                }
    }

    fun stopRecord() {
        recorder?.stopRecording()
    }

    fun play(context: Context) {
        if (recorder == null) {
            recorder = Recorder(context)
        }
        recorder?.playAudio()
                ?.observeOn(AndroidSchedulers.mainThread())
                ?.subscribe {
                    viewState.onEnergyLevel(it)
                }
    }

    fun stopPlayback() {
        recorder?.stopPlaying()
    }
}