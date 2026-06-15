
function p_LOS = get_probability_LOS(d, escenario)
%GET_PROBABILITY_LOS Returns the probability of LOS for a given distance and scenario.
%   Based on 3GPP TR 38.901 LOS probability models for different deployment types.
    % Define base station height in meters (only used in UMa)
    eNB_h = 25;

    switch escenario
        case 'RURAL_MACROCELL'
            if d <= 10
                p_LOS = 1;
            else
                p_LOS = exp(-1 * (d - 10) / 1000);
            end

        case 'URBAN_MICROCELL'
            if d <= 18
                p_LOS = 1;
            else
                p_LOS = 18/d + exp(-d/36) * (1 - 18/d);
            end

        case 'URBAN_MACROCELL'
            if d <= 18
                p_LOS = 1;
            else
                if eNB_h <= 13
                    C = 0;
                else
                    C = ((eNB_h - 13)/10)^1.5;
                end
                p_LOS = (18/d + exp(-d/63) * (1 - 18/d)) * (1 + C * (5/4) * (d/100)^3 * exp(-d/150));
            end

        case 'INDOOR_OPEN_OFFICE'
            if d <= 5
                p_LOS = 1;
            elseif d <= 49
                p_LOS = exp(-(d - 5) / 70.8);
            else
                p_LOS = exp(-(d - 49) / 211.7) * 0.54;
            end

        case 'INDOOR_MIXED_OFFICE'
            if d <= 1.2
                p_LOS = 1;
            elseif d <= 6.5
                p_LOS = exp(-(d - 1.2) / 4.7);
            else
                p_LOS = exp(-(d - 6.5) / 32.6) * 0.32;
            end

        case 'INDOOR_SHOPPING_MALL'
            p_LOS = (1 - 0.05) * exp(-d / 15.2) + 0.05;

        otherwise
            error('Escenario no válido.');
    end
end
