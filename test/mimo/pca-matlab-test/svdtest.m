%pkg load image
%pkg load linear-algebra

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
%[u1,s1,v1] = svd(M1 - repmat(mean(M1), size(M1, 1), 1)); %taking the svd of D will produce left singularvectors v..
%A=(M1')*(M1);
%[V,D]=eigs(A,10,'lm'); %..which are the same as the eigenvectors of D'*D

%test decoding stage

vectest1 = reshape(test, 1, 12*8); % generating test vectors
vectest2 = rand(1,10);
vectest3 = rand(1,10);
vectest4 = rand(1,10);
vectest5 = rand(1,13); %!!!
vectest6 = rand(1,10);
vectest7 = rand(1,10);
vectest8 = rand(1,10);
vectest9 = rand(1,12); %!!!
vectest10 = rand(1,10);

[u1, s1, v1, vtlm1, fw1, bw1, bw1test] = mypca('1', M1, vectest1); 
[u2, s2, v2, vtlm2, fw2, bw2, bw2test] = mypca('2', M2, vectest2); 
[u3, s3, v3, vtlm3, fw3, bw3, bw3test] = mypca('3', M3, vectest3); 
[u4, s4, v4, vtlm4, fw4, bw4, bw4test] = mypca('4', M4, vectest4); 
[u5, s5, v5, vtlm5, fw5, bw5, bw5test] = mypca('5', M5, vectest5); 
[u6, s6, v6, vtlm6, fw6, bw6, bw6test] = mypca('6', M6, vectest6); 
[u7, s7, v7, vtlm7, fw7, bw7, bw7test] = mypca('7', M7, vectest7); 
[u8, s8, v8, vtlm8, fw8, bw8, bw8test] = mypca('8', M8, vectest8); 
[u9, s9, v9, vtlm9, fw9, bw9, bw9test] = mypca('9', M9, vectest9, 12); 
[u10, s10, v10, vtlm10, fw10, bw10, bw10test] = mypca('10', M10, vectest10); 


%------------------------------------------------------------------------------
%file I/O

mkdir output;

mysave output/m1.txt M1 ;
mysave output/m2.txt M2;
mysave output/m3.txt M3;
mysave output/m4.txt M4;
mysave output/m5.txt M5;
mysave output/m6.txt M6;
mysave output/m7.txt M7;
mysave output/m8.txt M8;
mysave output/m9.txt M9;
mysave output/m10.txt M10;

mysave output/u1.txt u1;
mysave output/u2.txt u2;
mysave output/u3.txt u3;
mysave output/u4.txt u4;
mysave output/u5.txt u5;
mysave output/u6.txt u6;
mysave output/u7.txt u7;
mysave output/u8.txt u8;
mysave output/u9.txt u9;
mysave output/u10.txt u10;

mysave output/s1.txt s1;
mysave output/s2.txt s2;
mysave output/s3.txt s3;
mysave output/s4.txt s4;
mysave output/s5.txt s5;
mysave output/s6.txt s6;
mysave output/s7.txt s7;
mysave output/s8.txt s8;
mysave output/s9.txt s9;
mysave output/s10.txt s10;

mysave output/v1.txt v1;
mysave output/v2.txt v2;
mysave output/v3.txt v3;
mysave output/v4.txt v4;
mysave output/v5.txt v5;
mysave output/v6.txt v6;
mysave output/v7.txt v7;
mysave output/v8.txt v8;
mysave output/v9.txt v9;
mysave output/v10.txt v10;

mysave output/vlm1.txt vlm1;
mysave output/vlm2.txt vlm2;
mysave output/vlm3.txt vlm3;
mysave output/vlm4.txt vlm4;
mysave output/vlm5.txt vlm5;
mysave output/vlm6.txt vlm6;
mysave output/vlm7.txt vlm7;
mysave output/vlm8.txt vlm8;
mysave output/vlm9.txt vlm9;
mysave output/vlm10.txt vlm10;

mysave output/vectest1.txt vectest1;
mysave output/vectest2.txt vectest2;
mysave output/vectest3.txt vectest3;
mysave output/vectest4.txt vectest4;
mysave output/vectest5.txt vectest5;
mysave output/vectest6.txt vectest6;
mysave output/vectest7.txt vectest7;
mysave output/vectest8.txt vectest8;
mysave output/vectest9.txt vectest9;
mysave output/vectest10.txt vectest10;

mysave output/fw1.txt fw1;
mysave output/fw2.txt fw2;
mysave output/fw3.txt fw3;
mysave output/fw4.txt fw4;
mysave output/fw5.txt fw5;
mysave output/fw6.txt fw6;
mysave output/fw7.txt fw7;
mysave output/fw8.txt fw8;
mysave output/fw9.txt fw9;
mysave output/fw10.txt fw10;

mysave output/bw1test.txt bw1test;
mysave output/bw2test.txt bw2test;
mysave output/bw3test.txt bw3test;
mysave output/bw4test.txt bw4test;
mysave output/bw5test.txt bw5test;
mysave output/bw6test.txt bw6test;
mysave output/bw7test.txt bw7test;
mysave output/bw8test.txt bw8test;
mysave output/bw9test.txt bw9test;
mysave output/bw10test.txt bw10test;

mysave output/bw1.txt bw1;
mysave output/bw2.txt bw2;
mysave output/bw3.txt bw3;
mysave output/bw4.txt bw4;
mysave output/bw5.txt bw5;
mysave output/bw6.txt bw6;
mysave output/bw7.txt bw7;
mysave output/bw8.txt bw8;
mysave output/bw9.txt bw9;
mysave output/bw10.txt bw10;

%------------------------------------------------------------------------------
%visual computation of svd

vectest = reshape(test, 1, 12*8); %let´s test with another obama

%forward transformation to feature space
recon1 = vectest*vlm1; 
figure(1),
subplot(1,1,1), bar(recon1(1:10)), set(gca, 'Xlim',[0 20], 'Ylim', [-2000 2000], 'Xtick',[],'Ytick',[]),text(12,-1700, 'Obama * v', 'Fontsize',[15])

%backward transformation from feature space to image layed out on PC's
recon2 = recon1;
recon2(1,1) = recon2(1,1)*0.3; %doing some transformation on features
recon2(1,2) = recon2(1,2)*-0.5; %more transformation on features
proj1 = vlm1*(recon1'); %backward transform
proj2 =(recon2)*(vlm1'); %this is the same
face1=reshape(proj1,12,8);
face2=reshape(proj2,12,8);
figure(2),
subplot(3,2,1), pcolor(flipud(face1)), shading interp, colormap(gray), set(gca, 'Xtick', [], 'Ytick', [])
subplot(3,2,2), pcolor(flipud(face2)), shading interp, colormap(gray), set(gca, 'Xtick', [], 'Ytick', [])

break

