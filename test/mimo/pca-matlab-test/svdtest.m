pkg load image
pkg load linear-algebra

%The following input data is fed to the svd and tested.
%1: M*N rectangular matrix complex data
%2: M*M square matrix 
%3: M*M diagonal matrix
%4: M > N rectangular matrix
%5: M < N rectangular matrix
%6: Identity matrix
%7: Same rows square
%8: Same cols square
%9: Same rows rect
%10: Same cols rect

%The program outputs the data to text and is read by the C++ unit test and used
%as validated output for testing

%add some clooney pictures
C1 = imresize( double( rgb2gray( imread('input/clooney1.jpg', 'jpeg') ) ) , [12 8] );
C2 = imresize( double( rgb2gray( imread('input/clooney2.jpg', 'jpeg') ) ) , [12 8] );
C3 = imresize( double( rgb2gray( imread('input/clooney3.jpg', 'jpeg') ) ) , [12 8] );
C4 = imresize( double( rgb2gray( imread('input/clooney4.jpg', 'jpeg') ) ) , [12 8] );
C5 = imresize( double( rgb2gray( imread('input/clooney5.jpg', 'jpeg') ) ) , [12 8] );
%add some obama pictures
O1 = imresize( double( rgb2gray( imread('input/obama1.jpg', 'jpeg') ) ) , [12 8] );
O2 = imresize( double( rgb2gray( imread('input/obama2.jpg', 'jpeg') ) ) , [12 8] );
O3 = imresize( double( rgb2gray( imread('input/obama3.jpg', 'jpeg') ) ) , [12 8] );
O4 = imresize( double( rgb2gray( imread('input/obama4.jpg', 'jpeg') ) ) , [12 8] );
O5 = imresize( double( rgb2gray( imread('input/obama5.jpg', 'jpeg') ) ) , [12 8] );
%add testvector
test = imresize( double( rgb2gray( imread('input/obamatest.jpg', 'jpeg') ) ) , [12 8] );


%1: M*N rectangular matrix 
M1=[reshape(C1, 1, 12*8)
reshape(C2, 1, 12*8)
reshape(C3, 1, 12*8)
reshape(C4,1,12*8)
reshape(C5,1,12*8)
reshape(O1, 1, 12*8)
reshape(O2, 1, 12*8)
reshape(O3, 1, 12*8)
reshape(O4,1,12*8)
reshape(O5,1,12*8)];

%2: M*M square matrix
M2 = rand(10,10);

%3: M*M diagonal matrix
M3 = diag(M2(1:10) );

%4: M > N rectangular matrix
M4 = rand(12,10);

%5: M < N rectangular matrix
M5 = rand(10,13);

%6: Identity matrix
M6 = eye(10,10);

%7: Same rows square
VM7 = rand(10,1);
M7 = [VM7, VM7, VM7, VM7, VM7, VM7, VM7, VM7, VM7, VM7];

%8: Same cols square
VM8 = rand(1,10);
M8 = [VM8,
      VM8,
      VM8,
      VM8,
      VM8,
      VM8,
      VM8,
      VM8,
      VM8,
      VM8];
      
%9: Same rows rect
M9 = [VM7, VM7, VM7, VM7,VM7, VM7, VM7, VM7,VM7, VM7, VM7, VM7];

%10: Same cols rect
M10 = [VM8,
       VM8,
       VM8,
       VM8
       VM8,
       VM8,
       VM8,
       VM8,
       VM8,
       VM8,
       VM8,
       VM8];

%calculate SVD
[u1,s1,v1] = svd(M1); %taking the svd of D will produce left singularvectors v..
A=(M1')*(M1);
[V,D]=eigs(A,10,'lm'); %..which are the same as the eigenvectors of D'*D

[u2,s2,v2] = svd(M2); 
[u3,s3,v3] = svd(M3); 
[u4,s4,v4] = svd(M4); 
[u5,s5,v5] = svd(M5); 
[u6,s6,v6] = svd(M6);
[u7,s7,v7] = svd(M7);
[u8,s8,v8] = svd(M8);
[u9,s9,v9] = svd(M9);
[u10,s10,v10] = svd(M10);

vlm1 = v1(:,1:10); %select the 10 most prominent vectors
vlm2 = v2(:,1:10); 
vlm3 = v3(:,1:10); 
vlm4 = v4(:,1:10); 
vlm5 = v5(:,1:10); 
vlm6 = v6(:,1:10);
vlm7 = v7(:,1:10); 
vlm8 = v8(:,1:10); 
vlm9 = v9(:,1:12);
vlm10 = v10(:,1:10); 

vtlm1 = vlm1'; %calculate transposed matrices
vtlm2 = vlm2';
vtlm3 = vlm3';
vtlm4 = vlm4';
vtlm5 = vlm5';
vtlm6 = vlm6';
vtlm7 = vlm7';
vtlm8 = vlm8';
vtlm9 = vlm9';
vtlm10 = vlm10';

%test decoding stage

vectest1 = reshape(test, 1, 12*8); %generating test vectors
vectest2 = rand(1,10);
vectest3 = rand(1,10);
vectest4 = rand(1,10);
vectest5 = rand(1,13);
vectest6 = rand(1,10);
vectest7 = rand(1,10);
vectest8 = rand(1,10);
vectest9 = rand(1,12);
vectest10 = rand(1,10);

fw1 = vectest1*vlm1; %forward transformation
fw2 = vectest2*vlm2;
fw3 = vectest3*vlm3;
fw4 = vectest4*vlm4;
fw5 = vectest5*vlm5;
fw6 = vectest6*vlm6;
fw7 = vectest7*vlm7;
fw8 = vectest8*vlm8;
fw9 = vectest9*vlm9;
fw10 = vectest10*vlm10;

bw1test = fw1; %generating test vectors
bw2test = fw2;
bw3test = fw3;
bw4test = fw4;
bw5test = fw5;
bw6test = fw6;
bw7test = fw7;
bw8test = fw8;
bw9test = fw9;
bw10test = fw10;

bw1test(1,1) = bw1test(1,1)*rand(); %transforming the first principal feature
bw2test(1,1) = bw2test(1,1)*rand();
bw3test(1,1) = bw3test(1,1)*rand();
bw4test(1,1) = bw4test(1,1)*rand();
bw5test(1,1) = bw5test(1,1)*rand();
bw6test(1,1) = bw6test(1,1)*rand();
bw7test(1,1) = bw7test(1,1)*rand();
bw8test(1,1) = bw8test(1,1)*rand();
bw9test(1,1) = bw9test(1,1)*rand();
bw10test(1,1) = bw10test(1,1)*rand();

bw1 = bw1test * vtlm1; %backward transformation
bw2 = bw2test * vtlm2;
bw3 = bw3test * vtlm3;
bw4 = bw4test * vtlm4;
bw5 = bw5test * vtlm5;
bw6 = bw6test * vtlm6;
bw7 = bw7test * vtlm7;
bw8 = bw8test * vtlm8;
bw9 = bw9test * vtlm9;
bw10 = bw10test * vtlm10;

%------------------------------------------------------------------------------
%file I/O

mkdir output;

save output/m1.txt M1;
save output/m2.txt M2;
save output/m3.txt M3;
save output/m4.txt M4;
save output/m5.txt M5;
save output/m6.txt M6;
save output/m7.txt M7;
save output/m8.txt M8;
save output/m9.txt M9;
save output/m10.txt M10;

save output/u1.txt u1;
save output/u2.txt u2;
save output/u3.txt u3;
save output/u4.txt u4;
save output/u5.txt u5;
save output/u6.txt u6;
save output/u7.txt u7;
save output/u8.txt u8;
save output/u9.txt u9;
save output/u10.txt u10;

save output/s1.txt s1;
save output/s2.txt s2;
save output/s3.txt s3;
save output/s4.txt s4;
save output/s5.txt s5;
save output/s6.txt s6;
save output/s7.txt s7;
save output/s8.txt s8;
save output/s9.txt s9;
save output/s10.txt s10;

save output/v1.txt v1;
save output/v2.txt v2;
save output/v3.txt v3;
save output/v4.txt v4;
save output/v5.txt v5;
save output/v6.txt v6;
save output/v7.txt v7;
save output/v8.txt v8;
save output/v9.txt v9;
save output/v10.txt v10;

save output/vlm1.txt vlm1;
save output/vlm2.txt vlm2;
save output/vlm3.txt vlm3;
save output/vlm4.txt vlm4;
save output/vlm5.txt vlm5;
save output/vlm6.txt vlm6;
save output/vlm7.txt vlm7;
save output/vlm8.txt vlm8;
save output/vlm9.txt vlm9;
save output/vlm10.txt vlm10;

save output/vectest1.txt vectest1;
save output/vectest2.txt vectest2;
save output/vectest3.txt vectest3;
save output/vectest4.txt vectest4;
save output/vectest5.txt vectest5;
save output/vectest6.txt vectest6;
save output/vectest7.txt vectest7;
save output/vectest8.txt vectest8;
save output/vectest9.txt vectest9;
save output/vectest10.txt vectest10;

save output/fw1.txt fw1;
save output/fw2.txt fw2;
save output/fw3.txt fw3;
save output/fw4.txt fw4;
save output/fw5.txt fw5;
save output/fw6.txt fw6;
save output/fw7.txt fw7;
save output/fw8.txt fw8;
save output/fw9.txt fw9;
save output/fw10.txt fw10;

save output/bw1test.txt bw1test;
save output/bw2test.txt bw2test;
save output/bw3test.txt bw3test;
save output/bw4test.txt bw4test;
save output/bw5test.txt bw5test;
save output/bw6test.txt bw6test;
save output/bw7test.txt bw7test;
save output/bw8test.txt bw8test;
save output/bw9test.txt bw9test;
save output/bw10test.txt bw10test;

save output/bw1.txt bw1;
save output/bw2.txt bw2;
save output/bw3.txt bw3;
save output/bw4.txt bw4;
save output/bw5.txt bw5;
save output/bw6.txt bw6;
save output/bw7.txt bw7;
save output/bw8.txt bw8;
save output/bw9.txt bw9;
save output/bw10.txt bw10;

%------------------------------------------------------------------------------
%visual computation of svd

vectest = reshape(test, 1, 12*8); %let´s test with another obama

%forward transformation to feature space
recon1 = vectest*vlm1; 
figure(1),
subplot(1,1,1), bar(recon1(1:10)), set(gca, 'Xlim',[0 20], 'Ylim', [-2000 2000],
'Xtick',[],'Ytick',[]),text(12,-1700, 'Obama * v', 'Fontsize',[15])

%backward transformation from feature space to image layed out on PC's
recon2 = recon1;
recon2(1,1) = recon2(1,1)*0.3; %doing some transformation on features
recon2(1,2) = recon2(1,2)*-0.5; %more transformation on features
proj1 = vlm1*(recon1'); %backward transform
proj2 =(recon2)*(vlm1'); %this is the same
face1=reshape(proj1,12,8);
face2=reshape(proj2,12,8);
figure(2),
subplot(3,2,1), pcolor(flipud(face1)), shading interp, colormap(gray), set(gca, 
'Xtick', [], 'Ytick', [])
subplot(3,2,2), pcolor(flipud(face2)), shading interp, colormap(gray), set(gca, 
'Xtick', [], 'Ytick', [])

break

