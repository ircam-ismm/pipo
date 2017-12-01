pkg load image
test = imresize( double( rgb2gray( imread('obamatest.jpg', 'jpeg') ) ) , [120 80] );
%add some clooney
C1 = imresize( double( rgb2gray( imread('clooney1.jpg', 'jpeg') ) ) , [120 80] );
C2 = imresize( double( rgb2gray( imread('clooney2.jpg', 'jpeg') ) ) , [120 80] );
C3 = imresize( double( rgb2gray( imread('clooney3.jpg', 'jpeg') ) ) , [120 80] );
C4 = imresize( double( rgb2gray( imread('clooney4.jpg', 'jpeg') ) ) , [120 80] );
C5 = imresize( double( rgb2gray( imread('clooney5.jpg', 'jpeg') ) ) , [120 80] );
%add some obama
O1 = imresize( double( rgb2gray( imread('obama1.jpg', 'jpeg') ) ) , [120 80] );
O2 = imresize( double( rgb2gray( imread('obama2.jpg', 'jpeg') ) ) , [120 80] );
O3 = imresize( double( rgb2gray( imread('obama3.jpg', 'jpeg') ) ) , [120 80] );
O4 = imresize( double( rgb2gray( imread('obama4.jpg', 'jpeg') ) ) , [120 80] );
O5 = imresize( double( rgb2gray( imread('obama5.jpg', 'jpeg') ) ) , [120 80] );
%write contigiously in matrix
D=[reshape(C1, 1, 120*80)
reshape(C2, 1, 120*80)
reshape(C3, 1, 120*80)
reshape(C4,1,120*80)
reshape(C5,1,120*80)
reshape(O1, 1, 120*80)
reshape(O2, 1, 120*80)
reshape(O3, 1, 120*80)
reshape(O4,1,120*80)
reshape(O5,1,120*80)];

[u,s,v] = svd(D); %taking the svd of D will produce left singularvectors v..
A=(D')*(D);
[V,D]=eigs(A,20,'lm'); %..which are the same as the eigenvectors of D'*D
vmax20 = v(:,1:20); %select the 20 most prominent vectors

%------------------------------------------------------------------------------
vectest = reshape(test, 1, 120*80); %let´s test with another obama

%forward transformation to feature space
recon1 = vectest*vmax20; 
figure(1),
subplot(1,1,1), bar(recon1(1:20)), set(gca, 'Xlim',[0 20], 'Ylim', [-2000 2000],
'Xtick',[],'Ytick',[]),text(12,-1700, 'Obama * v', 'Fontsize',[15])

%backward transformation from feature space to image layed out on PC's
recon2 = recon1;
recon2(1,1) = recon2(1,1)*0.3; %doing some transformation on features
recon2(1,2) = recon2(1,2)*-0.5; %more transformation on features
proj1 = vmax20*(recon1'); %backward transform
proj2 =(recon1)*(vmax20'); %this is the same
face1=reshape(proj1,120,80);
face2=reshape(proj2,120,80);
figure(2),
subplot(3,2,1), pcolor(flipud(face1)), shading interp, colormap(gray), set(gca, 
'Xtick', [], 'Ytick', [])
subplot(3,2,2), pcolor(flipud(face2)), shading interp, colormap(gray), set(gca, 
'Xtick', [], 'Ytick', [])

break

