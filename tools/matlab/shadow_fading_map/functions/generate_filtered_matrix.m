function s = generate_filtered_matrix(corr_distance, dist, std_dev, n)
%GENERATE_FILTERED_MATRIX Generates a spatially correlated shadow fading map.
%   This method follows the approach from:
%   C. Zhang, “Two-Dimensional Shadow Fading Modeling on System Level,”
%   2012 IEEE 23rd International Symposium on Personal, Indoor and Mobile Radio Communications (PIMRC).
%
%   The function creates a shadow fading matrix with 2D spatial correlation,
%   using a filter derived from the spatial autocorrelation function.
%
%   Inputs:
%     corr_distance - Correlation distance in meters
%     dist          - Spatial resolution (cell size in meters)
%     std_dev       - Standard deviation of the shadow fading (in dB)
%     n             - Number of grid points per side (generates n x n map)
%
%   Output:
%     s             - n x n matrix of spatially correlated shadow fading values [dB]

    % Matrix center index
    center = ceil(n / 2);

    % Generate uncorrelated normal random field
    A = normrnd(0, std_dev, n, n);

    % Create spatial distance matrix for filter
    [X, Y] = meshgrid(1:n, 1:n);
    dist_matrix = R(dist * sqrt((X - center).^2 + (Y - center).^2), corr_distance);

    % Generate filter from the 2D Fourier domain
    H = sqrt(fft2(dist_matrix) / (std_dev^2));
    h = ifft2(H);

    % Extract central 11x11 region of the filter
    W = 11;  % Filter size (must be odd)
    start_idx = floor((n - W) / 2) + 1;
    end_idx = start_idx + W - 1;
    h = h(start_idx:end_idx, start_idx:end_idx);

    % Normalize the filter
    h = h / sqrt(sum(h(:).^2));

    % Apply the filter via 2D convolution to introduce spatial correlation
    s = conv2(A, h, 'same');
end
