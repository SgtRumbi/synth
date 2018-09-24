pushd ../build
g++ ../code/synth.cpp -o synth -lasound
g++ ../code/midi.cpp -o midi -lasound
popd
