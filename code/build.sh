pushd ../build
g++ ../code/synth.cpp -o synth -lasound
g++ ../code/midi.cpp -o midi
g++ ../code/kbtomidi.cpp -o kbtomidi -lX11
popd
