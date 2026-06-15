function [corr_distance_los, corr_distance_nlos, std_los, std_nlos] = adjust_parameters(escenario)
%ADJUST_PARAMETERS Returns scenario-specific shadow fading parameters.
%   This function sets the standard deviation (in dB) and correlation distance (in meters)
%   for both LOS and NLOS conditions.
%
%   - Shadowing standard deviations are summarized in Table 3-1 of the TFM.
%   - Decorrelation distances are taken from 3GPP TR 38.901 recommendations.
%
%   Input:
%     escenario - Scenario name as string:
%                 'RURAL_MACROCELL', 'URBAN_MICROCELL', 'URBAN_MACROCELL',
%                 'INDOOR_OPEN_OFFICE', 'INDOOR_MIXED_OFFICE', 'INDOOR_SHOPPING_MALL'
%
%   Outputs:
%     corr_distance_los   - Correlation distance for LOS [m]
%     corr_distance_nlos  - Correlation distance for NLOS [m]
%     std_los             - Shadow fading std dev in LOS [dB]
%     std_nlos            - Shadow fading std dev in NLOS [dB]

    switch escenario
        case 'RURAL_MACROCELL'
            corr_distance_los = 37;
            corr_distance_nlos = 120;
            std_los = 1.7;
            std_nlos = 6.7;

        case 'URBAN_MICROCELL'
            corr_distance_los = 10;
            corr_distance_nlos = 13;
            std_los = 4.3;
            std_nlos = 6.8;

        case 'URBAN_MACROCELL'
            corr_distance_los = 37;
            corr_distance_nlos = 50;
            std_los = 2.4;
            std_nlos = 5.3;

        case 'INDOOR_OPEN_OFFICE'
            corr_distance_los = 10;
            corr_distance_nlos = 6;
            std_los = 4.3;
            std_nlos = 7.9;

        case 'INDOOR_MIXED_OFFICE'
            corr_distance_los = 10;
            corr_distance_nlos = 6;
            std_los = 4.3;
            std_nlos = 7.9;

        case 'INDOOR_SHOPPING_MALL'
            corr_distance_los = 10;
            corr_distance_nlos = 10;
            std_los = 3.3;
            std_nlos = 4.6;

        otherwise
            error('Invalid scenario.');
    end
end
