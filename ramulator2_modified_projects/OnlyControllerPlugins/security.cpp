#include <vector>
#include <unordered_map>
#include <limits>
#include <random>
#include <queue>

#include "global_flags.h"
#include "base/base.h"
#include "dram_controller/bh_controller.h"
#include "dram_controller/plugin.h"
#include "dram_controller/impl/plugin/prac/prac.h"
#include "addr_mapper/impl/rit.h"

namespace Ramulator {

class Security : public IControllerPlugin, public Implementation {
    RAMULATOR_REGISTER_IMPLEMENTATION(IControllerPlugin, Security, "Security", "Security.")

    private:
        IDRAM* m_dram = nullptr;
        std::unordered_map<Addr_t, int> row_activation_counter; 
        int rh_th = -1; 
        int bitflip_count = 0; 
        int m_row_level = -1;
        int m_bank_level = -1;
        int m_rank_level = -1;
        
        int rfm_ref = 0;
        int vrr_ref = 0;
        int vrr_t_ref = 0;


        int m_vrr_t_req_id = -1;

        Clk_t m_clk = 0;

    public:
        void init() override {
            rh_th = param<int>("rh_threshold").default_val(500);
        }

        void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override {
            m_ctrl = cast_parent<IDRAMController>();
            m_dram = m_ctrl->m_dram;
            m_row_level = m_dram->m_levels("row");
            m_bank_level = m_dram->m_levels("bank");
            m_rank_level = m_dram->m_levels("rank");
            m_vrr_t_req_id = m_dram->m_requests("vrr-transitive");

            register_stat(bitflip_count).name("bitflip_count");
            register_stat(rh_th).name("rh_th");
            register_stat(rfm_ref).name("rfm_ref");
            register_stat(vrr_ref).name("vrr_ref");
            register_stat(vrr_t_ref).name("vrr_t_ref");


        }

        void update(bool request_found, ReqBuffer::iterator& req_it) override {
            m_clk++;
            if (request_found) {
                if (m_dram->m_command_meta(req_it->command).is_opening && 
                    m_dram->m_command_scopes(req_it->command) == m_row_level) {
                    Addr_t row_id = req_it->addr_vec[m_row_level];
                    row_activation_counter[row_id + 1] ++;
                    row_activation_counter[row_id - 1] ++;
                
                    if (row_activation_counter[row_id + 1] >= rh_th) {
                        bitflip_count++;
                        row_activation_counter[row_id + 1] = 0;
                    }
                    if (row_activation_counter[row_id - 1] >= rh_th) {
                        bitflip_count++;
                        row_activation_counter[row_id - 1] = 0;
                    }
                }
                if (global_vrr_flag > 0) {
                    Addr_t row_id = req_it->addr_vec[m_row_level];
                    vrr_ref ++;
                    row_activation_counter[row_id + 1] = 0;
                    row_activation_counter[row_id - 1] = 0;
                    row_activation_counter[row_id + 2] ++;
                    row_activation_counter[row_id - 2] ++;
                    global_vrr_flag --; 
                }
                if (global_vrr_t_flag > 0) {
                    Addr_t row_id = req_it->addr_vec[m_row_level];
                    vrr_t_ref ++;
                    row_activation_counter[row_id + 2] = 0;
                    row_activation_counter[row_id - 2] = 0;
                    global_vrr_t_flag --; 
                }
                if (req_it->command == m_vrr_t_req_id) {
                    rfm_ref ++;
                    int flat_bank_id = req_it->addr_vec[m_bank_level];
                    int accumulated_dimension = 1;
                    for (int i = m_bank_level - 1; i >= m_rank_level; i--) {
                        accumulated_dimension *= m_dram->m_organization.count[i + 1];
                        flat_bank_id += req_it->addr_vec[i] * accumulated_dimension;
                    }

                    int row_start = flat_bank_id * m_dram->get_level_size("row");
                    int row_end = row_start + m_dram->get_level_size("row");

                    for (int row = row_start; row < row_end; row++) {
                        row_activation_counter.erase(row);
                    }
                }
            }
            
            if (m_clk % 64000000 == 0){
                row_activation_counter.clear();
            }
            
        }
};
}

