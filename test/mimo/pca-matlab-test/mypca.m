% train, forward, backward, write all data

function [U, S, vlm, vtlm, fw, bw, bwtest] = mypca(label, m, vectest, N)
    disp('')
    disp(['----- data set ', label]);
    
    mu  = mean(m, 1);
    MU  = repmat(mu, size(m, 1), 1);          % means matrix, expanded over rows
    [U, S, V] = svd(m - MU); %, 'econ');
    S = diag(S)
    S = S(1:max(find(abs(S) > 0.7e-15))) % remove trailing zeros from diagonal of S, threshold tweaked exactly to this dataset, to reproduce sgesvd on Mac's behaviour
    
    % fw = (m - MU) * V
    % bw = fw * V' + MU

    if nargin < 4, 
        N = 10; 
    end
    
    vlm = V(:,1:N);                     % select the 10 most prominent vectors
    vtlm = vlm';                        % calculate transposed matrices

    fw = (vectest - mu) * vlm;          % forward transformation
    
    bwtest = fw;                        % generating test vectors
    bwtest(1,1) = bwtest(1,1) * rand(); % transforming the first principal feature
    bw = bwtest * vtlm + mu;            % backward transformation
