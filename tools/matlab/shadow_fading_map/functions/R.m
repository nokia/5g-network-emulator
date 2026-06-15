function correlation= R(dist, corr_distance)
   %it is the correlation function described in 3GPP 38.901
    correlation = exp(-dist / corr_distance);
end