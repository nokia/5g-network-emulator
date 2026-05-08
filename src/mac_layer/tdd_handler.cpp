#include <mac_layer/tdd_handler.h>
#include <phy_layer/phy_l_definitions.h>

tdd_handler::tdd_handler(int n_dl_slots, int n_ul_slots, int transition_c, int _tx)
{
    if(!(transition_c > 0 && transition_c < N_CONFIG)) transition_c = FALLBACK_CONFIG;
    for(int i = 0; i < n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
    for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]);
    for(int i = 0; i < n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
    tx = _tx;
    dl_slots = n_dl_slots;
    ul_slots = n_ul_slots;
    t_config = transition_c;
    LOG_INFO_I("tdd_handler::tdd_handler") << "TDD Configuration DL [" << n_dl_slots << "]  UL [" << n_ul_slots << "] Transition [" << transition_c  << "] [";
    for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[transition_c][i]; j++) std::cout << TDD_ORDER[i];
    LOG_INFO() << "]" << END();
}

tdd_handler::tdd_handler(tdd_config tdd_c, int _tx)
{
    if(!(tdd_c.transition_c > 0 && tdd_c.transition_c < N_CONFIG)) tdd_c.transition_c = FALLBACK_CONFIG;
    for(int i = 0; i < tdd_c.n_dl_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[0]);
    for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[tdd_c.transition_c][i]; j++) tdd_scheme.push_back(TDD_ORDER[i]);
    for(int i = 0; i < tdd_c.n_ul_slots*N_5G_SYMS; i++) tdd_scheme.push_back(TDD_ORDER[2]);
    tx = _tx;
    dl_slots = tdd_c.n_dl_slots;
    ul_slots = tdd_c.n_ul_slots;
    t_config = tdd_c.transition_c;
    LOG_INFO_I("tdd_handler::tdd_handler") << "TDD Configuration DL [" << tdd_c.n_dl_slots << "]  UL [" << tdd_c.n_ul_slots << "] Transition [" << tdd_c.transition_c  << "] [";
    for(int i = 0; i < 3; i++) for(int j = 0; j < TDD_5G_CONFIGURATION[tdd_c.transition_c][i]; j++) std::cout << TDD_ORDER[i];
    LOG_INFO() << "]" << END();
}

void tdd_handler::set_syms(int _syms)
{
    syms = _syms;
}

int tdd_handler::get_syms()
{
    int syms_out = 0;
    int index = c_index;
    for(int i = 0; i < syms; i++)
    {
        if(index >= (int)tdd_scheme.size()) index = 0;
        if(tdd_scheme[index] == UL_SYM && tx == TX_UL) syms_out++;
        if(tdd_scheme[index] == DL_SYM && tx == TX_DL) syms_out++;
        index += 1;
    }
    c_index = index;
    return syms_out;
}

void tdd_handler::step()
{
    c_index += syms;
}
