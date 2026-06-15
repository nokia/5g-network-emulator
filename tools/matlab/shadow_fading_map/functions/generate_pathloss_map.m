function pathloss_map = generate_pathloss_map(scenario, los, dist, n, freq_ghz)
%GENERATE_PATHLOSS_MAP Computes a 2D pathloss map using the ABG model.
%   This function calculates the large-scale pathloss for a given scenario using
%   the ABG (Alpha-Beta-Gamma) model, where:
%     - alpha models distance-dependent loss,
%     - beta is the offset at 1 meter,
%     - gamma controls the frequency dependency.
%
%   Inputs:
%     scenario  - Deployment type. Accepted values:
%                'RURAL_MACROCELL', 'URBAN_MACROCELL', 'URBAN_MICROCELL',
%                'INDOOR_OPEN_OFFICE', 'INDOOR_MIXED_OFFICE', 'INDOOR_SHOPPING_MALL'
%     los       - Boolean flag (true for LOS, false for NLOS)
%     dist      - Spatial resolution in meters (pixel size)
%     n         - Number of cells per side (generates n x n map)
%     freq_ghz  - Frequency in GHz
%
%   Output:
%     pathloss_map - n x n matrix with pathloss values in dB

    % Generate grid of distances from the center
    x = dist * ((1:n) - ceil(n / 2));
    [X, Y] = meshgrid(x, x);
    distances = sqrt(X.^2 + Y.^2);
    distances(distances == 0) = 1e-6;  % Avoid log(0)

    % Set ABG parameters based on scenario and LOS/NLOS
    switch scenario
        case 'RURAL_MACROCELL'
            if los
                alpha = 2.16; beta = 32.4; gamma = 2;
            else
                alpha = 2.75; beta = 32.4; gamma = 2;
            end

        case 'URBAN_MACROCELL'
            if los
                if freq_ghz < 6
                    alpha = 2.2; beta = 28; gamma = 2;
                else
                    alpha = 1.9; beta = 35.8; gamma = 1.9;
                end
            else
                alpha = 3.5; beta = 13.6; gamma = 2.4;
            end

        case 'URBAN_MICROCELL'
            if los
                if freq_ghz < 6
                    alpha = 2.27; beta = 27.02; gamma = 2;
                else
                    alpha = 1.1; beta = 46.8; gamma = 2.1;
                end
            else
                alpha = 2.8; beta = 31.4; gamma = 2.7;
            end

        case {'INDOOR_OPEN_OFFICE', 'INDOOR_MIXED_OFFICE'}
            if los
                if freq_ghz < 6
                    alpha = 1.87; beta = 32.82; gamma = 2;
                else
                    alpha = 1.6; beta = 32.9; gamma = 1.8;
                end
            else
                if freq_ghz < 6
                    alpha = 4.33; beta = 11.5; gamma = 2;
                else
                    alpha = 3.9; beta = 19; gamma = 2.1;
                end
            end

        case 'INDOOR_SHOPPING_MALL'
            if los
                alpha = 1.9; beta = 31.2; gamma = 2.2;
            else
                alpha = 2.0; beta = 34.4; gamma = 2.3;
            end

        otherwise
            error('Unsupported scenario: %s', scenario);
    end

    % Compute pathloss using the ABG model
    pathloss_map = 10 * alpha * log10(distances) + beta + 10 * gamma * log10(freq_ghz);

    % Post-processing: clean invalid entries
    pathloss_map(isnan(pathloss_map) | isinf(pathloss_map)) = Inf;
    pathloss_map(distances == 1e-6) = 0;  % Set center point to 0 dB
end
