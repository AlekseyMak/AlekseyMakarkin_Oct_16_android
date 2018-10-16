package com.amakarkin.alanchallenge.domain

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import io.reactivex.Observable
import kotlin.math.log
import kotlin.math.log10

class Recorder {

    companion object {
        private val SAMPLE_RATE = 44100
        private val TAG = Recorder::class.java.simpleName
    }

    private var shouldContinue = false

    //return energy level in normalized fashion. 0..100
    fun recordAudio(): Observable<Int> {
        return Observable.create { emitter ->
            shouldContinue = true
            Thread(Runnable {
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_AUDIO)

                var bufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE,
                        AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT)

                if (bufferSize == AudioRecord.ERROR || bufferSize == AudioRecord.ERROR_BAD_VALUE) {
                    bufferSize = SAMPLE_RATE * 2
                }

                val audioBuffer = ShortArray(bufferSize / 2)

                val record = AudioRecord(MediaRecorder.AudioSource.MIC,
                        SAMPLE_RATE,
                        AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        bufferSize)

                if (record.state != AudioRecord.STATE_INITIALIZED) {
                    Log.e(TAG, "Audio Record can't initialize!")
                    return@Runnable
                }
                record.startRecording()

                Log.v(TAG, "Start recording")

                var shortsRead: Long = 0
                var energyLevel: Double

                while (shouldContinue) {
                    if (emitter.isDisposed) {
                        shouldContinue = false
                        break
                    }
                    val numberOfShort = record.read(audioBuffer, 0, audioBuffer.size)
                    shortsRead += numberOfShort.toLong()

                    // Do something with the audioBuffer
                    Log.i(TAG, "Shorts: ${shortsRead}, Diff is: ${numberOfShort}")

                    //map raw values to db format.
                    energyLevel = audioBuffer.fold(0.0) { acc, sample -> acc + sample * sample } / audioBuffer.size
                    Log.i(TAG, "${energyLevel}")
                    energyLevel = 20*log10(energyLevel / 4000)
                    emitter.onNext(energyLevel.toInt())
                }

                record.stop()
                record.release()
                Log.v(TAG, String.format("Recording stopped. Samples read: %d", shortsRead))
                emitter.onComplete()
            }).start()
        }
    }

    fun stopRecording() {
        shouldContinue = false
    }

}