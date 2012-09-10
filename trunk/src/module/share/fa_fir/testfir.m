[x,fs]=wavread('1m.wav');
h = fir1(30, 0.5, 'low');
y = conv(x,h);
wavwrite(y,fs,'1m_out.wav');