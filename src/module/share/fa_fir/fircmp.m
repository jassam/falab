N=30;
delay=((N-1)/2);
offset = floor(delay);

fc=0.3;
i=[0:1:N-1];
w=hamming(N);
%w=blackman(N);
%w=kaiser(N,8.96)
w=w';

h=(fc*sin(pi*fc*(i-delay))./(pi*fc*(i-delay))).*w;
h(offset+1)=fc;
h1=fir1(N-1,fc,w);
subplot(2,1,1);
plot(h);
subplot(2,1,2);
plot(h1);

h2=-(fc*sin(pi*fc*(i-delay))./(pi*fc*(i-delay))).*w;
h2(offset)=1-fc;
h3=fir1(N-1,fc,'high');
figure(2);
subplot(2,1,1)
plot(h2);
subplot(2,1,2);
plot(h3);

