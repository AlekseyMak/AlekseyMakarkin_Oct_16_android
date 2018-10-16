package com.amakarkin.alanchallenge.presentation

import com.arellomobile.mvp.InjectViewState
import com.arellomobile.mvp.MvpPresenter
import com.arellomobile.mvp.MvpView

interface MainView : MvpView {

}

@InjectViewState
class MainPresenter : MvpPresenter<MainView>() {
}