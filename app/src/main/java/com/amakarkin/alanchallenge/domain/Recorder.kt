package com.amakarkin.alanchallenge.domain

import android.content.Context
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import io.reactivex.Observable
import java.io.File
import android.media.AudioTrack
import android.media.AudioManager
import io.reactivex.schedulers.Schedulers
import java.lang.IllegalStateException


class Recorder(context: Context) {

    companion object {
        private val SAMPLE_RATE = 44100
        private val TAG = Recorder::class.java.simpleName
        private val FILE_NAME = "raw.audio"
    }

    private var shouldContinue = false
    private var record: AudioRecord? = null
    private var audioTrack: AudioTrack? = null
    private lateinit var audioBuffer: ShortArray
    private var filePath: String

    init {
        val path = File(context.filesDir, "audio")
        if (!path.exists()) {
            path.mkdir()
        }
        filePath = "${path.absolutePath}/$FILE_NAME"
    }

    fun stopPlaying() {
        shouldContinue = false
    }

    fun stopRecording() {
        shouldContinue = false
    }

    fun recordAudio(): Observable<Int> {
        return Observable.create { emitter ->
            shouldContinue = true
            Thread(Runnable {
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO)

                if (!prepareRecorder()) {
                    emitter.onError(IllegalStateException("Failed to prepare recorder"))
                    return@Runnable
                }

                record?.startRecording()

                Log.v(TAG, "Start recording")

                var shortsRead: Long = 0

                while (shouldContinue) {
                    if (emitter.isDisposed) {
                        shouldContinue = false
                        break
                    }
                    val numberOfShort = record!!.read(audioBuffer, 0, audioBuffer.size)
                    shortsRead += numberOfShort.toLong()

                    emitter.onNext(processFrame(audioBuffer))
                }

                record?.stop()
                record?.release()
                stopRecordingNative()
                Log.v(TAG, String.format("Recording stopped. Samples play: %d", shortsRead))
                emitter.onComplete()
            }).start()
        }
    }

    fun changeTempo(speedFactor: Double) {
        audioTrack?.playbackRate = Math.round(SAMPLE_RATE * speedFactor).toInt()
    }

    fun playAudio(): Observable<Int> {
        val energyObserver: Observable<Int> = Observable.create { emitter ->

            if (!preparePlayer()) {
                emitter.onError(IllegalStateException("Failed to prepare player"))
                return@create
            }

            audioTrack?.play()

            Log.v(TAG, "Audio streaming started")

            var buffer = readNative()
            while (buffer != null && shouldContinue) {
                audioTrack?.write(buffer, 0, buffer.size)
                emitter.onNext(computeEnergyLevel(buffer))
                buffer = readNative()
            }

            closeInputNative()
            audioTrack?.release()

            Log.v(TAG, "Audio streaming finished.")
            emitter.onComplete()
        }
        return energyObserver.subscribeOn(Schedulers.computation())
    }

    private fun preparePlayer(): Boolean {
        shouldContinue = true
        if (!prepareInputNative(filePath)){
            return false
        }

        var bufferSize = AudioTrack.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT)
        if (bufferSize == AudioTrack.ERROR || bufferSize == AudioTrack.ERROR_BAD_VALUE) {
            bufferSize = SAMPLE_RATE * 2
        }

        audioTrack = AudioTrack(
                AudioManager.STREAM_MUSIC,
                SAMPLE_RATE,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM)

        return true
    }

    private fun prepareRecorder(): Boolean {
        var bufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT)

        if (bufferSize == AudioRecord.ERROR || bufferSize == AudioRecord.ERROR_BAD_VALUE) {
            bufferSize = SAMPLE_RATE * 2
        }

        audioBuffer = ShortArray(bufferSize / 2)

        record = AudioRecord(MediaRecorder.AudioSource.MIC,
                SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize)

        if (record?.state != AudioRecord.STATE_INITIALIZED) {
            Log.e(TAG, "Audio Record can't initialize!")
            return false
        }

        return prepareNative(filePath)
    }

    private external fun prepareInputNative(filePath: String): Boolean
    private external fun readNative(): ShortArray?
    private external fun closeInputNative()
    private external fun prepareNative(filePath: String): Boolean
    private external fun processFrame(buffer: ShortArray): Int
    private external fun stopRecordingNative()
    private external fun computeEnergyLevel(frame: ShortArray): Int

}