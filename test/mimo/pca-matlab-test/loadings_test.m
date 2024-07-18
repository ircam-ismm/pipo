format compact

% lozenge
m = [0, 0,
     1, 0,
     1, 1,
     2, 1]

cla, hold on
plot(m(:,1), m(:,2), 'o')
xlim([-1, 2])
ylim([-1, 2])

[p,score,latent,tsquared,explained,mu] = pca(m)
% m is data X(M, N)
% p are loadings V, score is T = X transformed into pca space, latent is S diagonal
% X = U S Vt
% T = X V = U S Vt V = U S
% https://en.wikipedia.org/wiki/Principal_component_analysis#Singular_value_decomposition

% draw loadings as vectors (from mu)
for i = 1:2,
    plot(mu(1) + [0, p(i, 1)], mu(2) + [0, p(i, 2)], 'r->')
end

% compare to centered svd: V = loadings p, latent lambda = s^2 / (M - 1)
[U,S,V] = svd(m - repmat(mean(m, 1), size(m, 1), 1))

% verify transposed call
[Vt,St,Ut] = svd((m - repmat(mean(m, 1), size(m, 1), 1))')

% https://netlib.org/lapack/explore-html/d1/d7f/group__gesvd_ga23ab797a9c7feb13b3f6feb0b52673b8.html#ga23ab797a9c7feb13b3f6feb0b52673b8

     % SGESVD computes the singular value decomposition (SVD) of a real
     % M-by-N matrix A, optionally computing the left and/or right singular
     % vectors. The SVD is written

     %      A = U * SIGMA * transpose(V)

     % where SIGMA is an M-by-N matrix which is zero except for its
     % min(m,n) diagonal elements, U is an M-by-M orthogonal matrix, and
     % V is an N-by-N orthogonal matrix.  The diagonal elements of SIGMA
     % are the singular values of A; they are real and non-negative, and
     % are returned in descending order.  The first min(m,n) columns of
     % U and V are the left and right singular vectors of A.

     % Note that the routine returns V**T, not V.

% test forward transform
MU  = repmat(mean(m, 1), size(m, 1), 1);          % means matrix, expanded over rows
fw  = (m - MU) * p
bw  = score * p' + MU
bws = fw * V' + MU
err = [ sum(abs(fw - score)), sum(abs(bw - m)), sum(abs(bws - m)) ]
