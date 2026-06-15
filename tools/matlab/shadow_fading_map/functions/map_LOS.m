function mapa_los = map_LOS(dist, n, escenario)
%MAP_LOS Generates a spatial LOS probability map based on distance and scenario.
%   Each point is assigned a probability of LOS based on its distance to the center.
%   A random threshold map is then used to stochastically determine LOS/NLOS.
%   Finally, a Gaussian filter is applied to introduce spatial correlation.

    centro_fila = ceil(n / 2);
    centro_columna = ceil(n / 2);

    distancias = zeros(n, n);
    probability_LOS = zeros(n, n);

    % Compute distance and LOS probability for each cell
    for fila = 1:n
        for columna = 1:n
            distancias(fila, columna) = dist * sqrt((fila - centro_fila)^2 + (columna - centro_columna)^2);
            probability_LOS(fila, columna) = get_probability_LOS(distancias(fila, columna), escenario);
        end
    end

    % Random threshold map for LOS assignment
    thres = rand(n, n);
    raw_map = thres <= probability_LOS;

    % Apply Gaussian filter to add spatial correlation
    sigma = 1.5;
    gauss_filter = fspecial('gaussian', [n n], sigma);
    mapa_los = imfilter(raw_map, gauss_filter, 'symmetric');
end
