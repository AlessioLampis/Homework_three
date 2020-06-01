( //#1 ======INITIALIZE SC SERVER ======
//if server fails to start and error encountered:  try, try again...it's stubborn sometimes...
o = Server.local.options;
o.sampleRate = 44100; //
o.blockSize = 512; //
o.inDevice = nil; //use system default audio input (change value to manually configure input)
o.outDevice = nil; //use system default audio output (change value to manually configure output)
s.latency = 0.05; //0.2 is SC default
s.boot;
)
// ALT + 126 for tilde
//OSC In/Out ----------
//~oscOut_JUCE = NetAddr("127.0.0.1", 6448); // TO FIND THE ADDR for JUCE


//BUSSES

~bus_input1 = Bus.audio(s, 1); // input bus: it takes the mono signal of the voice
~bus_pitch = Bus.control(s, 1); //pitch detection via Pitch function
~bus_dryIn1Amp = Bus.control(s, 1); //dry input level bus it is needed to gain the clean signal
~bus_vocSynthSum = Bus.audio(s, 2); //last synth bus where dry channel and shifted synths are fed into it
~bus_drywet = Bus.control(s,1); //amount of dry wet
~bus_vocDryStereo = Bus.audio(s, 2); // Input stereo bus
~bus_vocHarmMono = Bus.audio(s, 1); //mono bus of the harmonizer

//SYNTH DEFS ----------

(
SynthDef.new(\monAudioInput, {
	arg inChannel = 0, amp = 1, outBus;
	var sig = SoundIn.ar(bus: inChannel, mul: amp); //defaults to first channel of system audio input
	OffsetOut.ar(outBus, sig);
}).add;

SynthDef.new(\Pitch, {
	arg input, ampThresh = 0.1, median = 7, minFreq = 60, maxFreq = 4000, outBus;
	var sigIn, freq, hasFreq;
	sigIn = In.ar(input);
	# freq, hasFreq = Pitch.kr(sigIn, ampThreshold: ampThresh, median: median, minFreq: minFreq, maxFreq: maxFreq);
	//#freq, hasFreq = Tartini.kr(sigIn,0.9,2048);
	Out.kr(outBus, freq.cpsmidi.round(1));
}).add;

SynthDef.new(\mono2stereo, { //pass mono input to stereo output
	|in, amp = 1, pan = 0.0, out|
	var sig;
	sig = In.ar(in) * amp;
	sig = Pan2.ar(sig, pan);
	Out.ar(out, sig);
} ).add;

SynthDef.new(\mono2stereoHarm, { //pass mono input to stereo output
	|in, amp = 1, pan = 0.0, out|
	var sig;
	sig = In.ar(in) * amp;
	sig = Pan2.ar(sig, pan);
	Out.ar(out, sig);
} ).add;

SynthDef.new(\XFadeStereo, { //mix the wet and dry signal
	|in1, in2, out, amountControlBus, amp = 1.0|
	var inArray, mixSignal, sig;
	inArray = [In.ar(in1, 2), In.ar(in2, 2)];
	mixSignal = SelectX.ar(amountControlBus, inArray);
	sig = mixSignal * amp;
	Out.ar(out, sig);
} ).add;

SynthDef.new(\reverb2x2, {| outBus, mix = 0.25, room = 0.25, damp = 0.2, amp = 1.0|
	var sigIn, sigOut;
	sigIn = In.ar(outBus, 2);
	sigOut = FreeVerb2.ar(sigIn[0], sigIn[1], mix, room, damp, amp);
	ReplaceOut.ar(outBus, sigOut);
} ).add;


SynthDef.new(\vocHarm, {
	| num, pitchIn, sigIn, freq, in1, in2, pitchRatio, sigOut, audioOut, interval, mix , pan= 0.5|
		sigIn = In.ar(in1, 1); // CLEAN VOICE mono
	    freq = In.kr(in2); // the frequency of the voice
		pitchIn = freq;  //midi of my voice
		interval = num - pitchIn; //interval in midi
		pitchRatio = interval.asFloat.round(0.01).midiratio; //set pitch transposition (semi-tone intervals)
		sigOut = SelectX.ar(mix, [sigIn, PitchShift.ar(sigIn, 0.1, pitchRatio, 0, 0.002)]);
	    sigOut = BPF.ar(sigOut, 1000.0, 0.9 );
	sigOut = BPeakEQ.ar(sigOut, freq: 800.0, rq: 0.3, db: 6.0, mul: 1.0, add: 0.0);
	//sigOut = Compander.ar(in: sigOut, control: 0.0, thresh: 0.1, slopeBelow: 1.0, slopeAbove: 1.0, clampTime: 0.01, relaxTime: 0.1, mul: 1.0, add: 0.0);
	Out.ar(audioOut, sigOut);
	}).add;
)



//BUSSES and reverb SETUP
~bus_dryIn1Amp.set(0.9); //input gain
~bus_drywet.set(0.9); //initial value of drywet


//Synth activation

i = Synth(\monAudioInput, [\outBus, ~bus_input1]); //stream your system's first audio input channel to an SC audio bus (NOTE: THIS SYNTH MUST BE ACTIVE BEFORE ENABLING ANY OTHER FEATURE EXTRACTOR SYNTH BELOW)

p = Synth(\Pitch, [\input, ~bus_input1, \outBus, ~bus_pitch]).moveAfter(i); //outputs one value (freq)

~dryIn1 = Synth(\mono2stereo, [\in, ~bus_input1, \amp, ~bus_dryIn1Amp.asMap, \out, ~bus_vocDryStereo]).moveAfter(i);

~pitchShifter= Synth(\vocHarm, [\in1,~bus_input1,  \in2, ~bus_pitch, \num, ~bus_pitch.asMap, \audioOut, ~bus_vocHarmMono, \mix, 1 ]).moveAfter(~dryIn1); //48 is C3 and 69 is 440Hz

~harmStereo = Synth(\mono2stereoHarm, [\in,  ~bus_vocHarmMono, \amp, ~bus_dryIn1Amp.asMap, \out,~bus_vocSynthSum], addAction:\addToTail).moveAfter(~pitchShifter);

~accomp_mix= Synth(\XFadeStereo, [\in2, ~bus_vocSynthSum, \in1, ~bus_vocDryStereo,\amountControlBus,~bus_drywet.asMap,  \out, 0], addAction:\addToTail).moveAfter(~harmStereo);

~fx_reverb = Synth(\reverb2x2, [\outBus, 0], addAction: \addToTail).moveAfter(~accomp_mix); //stereo reverb processing sum output

//CLEANUP
(
i.free;
p.free;
~dryIn1.free;
~pitchShifter.free;
~harmStereo .free;
~accomp_mix.free;
)

//OSC communication

n = NetAddr("127.0.0.1");
NetAddr.localAddr;
NetAddr.langPort;
SerialPort.devices;

//OSCFunc.newMatching({|msg, time, addr,recvPort| ~pitchShifter.set(\num,msg[1]).postln},'/vocHarm/num', n);


OSCFunc.newMatching({|msg, time, addr,recvPort|~bus_drywet.set(msg[1]).postln},'/~bus_drywet', n);
OSCFunc.newMatching({|msg, time, addr,recvPort|~bus_dryIn1Amp.set(msg[1]).postln},'/~bus_dryIn1Amp', n);
OSCFunc.newMatching({|msg, time, addr,recvPort| ~fx_reverb.set(\mix,msg[1]).postln},'/reverbAmount', n);
OSCFunc.newMatching({|msg, time, addr,recvPort| ~fx_reverb.set(\room,msg[1]).postln},'/room', n);



/*
b = NetAddr.new("127.0.0.1", 7771);    // create the NetAddr
b.sendMsg("/hello", "there");    // send the application the message "hello" with the parameter "there"
*/
// DEBUG
(
 ~bus_pitch.value; // see the bus information
~bus_pitch.getnSynchronous(1)[0];
~pitchShifter.map(\in1, ~bus_input1 ); //FAILURE IN SERVER /n_mapan Node 1019 not found
~pitchShifter.trace;
); //return the current value




//Midi Setup //
(
var notes, on, off; // must be set as global variable
MIDIClient.init;
MIDIIn.connectAll;
notes = Array.newClear(128);    // array has one slot per possible MIDI note

 MIDIFunc.noteOn({
		arg veloc, num, chan, src;
	    notes[num] = Synth(\vocHarm, [\in1,~bus_input1,  \in2, ~bus_pitch, \num, num, \audioOut, ~bus_vocHarmMono, \mix, 1 ]).moveAfter(i);
});

off = MIDIFunc.noteOff({
	arg veloc, num, chan, src;
	notes[num].free;
	notes[num] = nil;

});
q = { on.free; off.free; };

//MIDIFunc.trace(bool: true); // important for DEBUGGING
)





