package com.amakarkin.alanchallenge.domain

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.util.Log
import io.reactivex.Observable

class Recorder {

    companion object {
        private val SAMPLE_RATE = 44100
        private val TAG = Recorder::class.java.simpleName
    }

    private var shouldContinue = false

    //return energy level
    fun recordAudio(): Observable<Double> {
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
                while (shouldContinue) {
                    if (emitter.isDisposed) {
                        shouldContinue = false
                        break
                    }
                    val numberOfShort = record.read(audioBuffer, 0, audioBuffer.size)
                    shortsRead += numberOfShort.toLong()

                    // Do something with the audioBuffer
                    Log.i(TAG, "Shorts: ${shortsRead}, Diff is: ${numberOfShort}, total: ${audioBuffer.fold(0, { i, a -> i + a })}")

                    emitter.onNext(audioBuffer.last().toDouble())
                }

                record.stop()
                record.release()
                Log.v(TAG, String.format("Recording stopped. Samples read: %d", shortsRead))
                emitter.onComplete()
            }).start()
        }
    }

}