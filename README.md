Create an Android application with the following properties:
1. A button that toggles sound recording from the microphone.
2. Raw audio frames should be recorded in Java code and sent to the Native code via NDK.
3. Frames must be stored in native code.
4. The application must calculate the Energy Level of the audio stream in Native and send it back
to Java layer where it must be displayed graphically.
5. Sound energy level can be computed per audio frame as the sum of squares of sample values
divided by the number of samples per frame.

Bonus feature 1.
6. There should be a playback button, and by clicking on this Java code starts the process of
reading frames from the Native part and playing them.
7. In the process of playing there should be real-time Sound energy level indication.

Bonus feature 2.
8. A slider UI element that changes the speed of playback.

All UI designs are up to you. If you need to use third party libraries
to process voice and they should be in Native code.