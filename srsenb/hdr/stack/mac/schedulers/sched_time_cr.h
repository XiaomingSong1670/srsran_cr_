#ifndef SRSRAN_SCHED_TIME_CR_H
#define SRSRAN_SCHED_TIME_CR_H

#include "sched_base.h"
#include "srsenb/hdr/common/common_enb.h"
#include "srsran/adt/circular_map.h"
#include <queue>


namespace srsenb {
    
class sched_time_cr final : public sched_base
{
    using ue_cit_t = sched_ue_list::const_iterator;

public:
    sched_time_cr(const sched_cell_params_t& cell_params_, const sched_interface::sched_args_t& sched_args); // initialize scheduler for ues
    void sched_dl_users(sched_ue_list& ue_db, sf_sched* tti_sched) override; // generate downlink users bitmasks
    void sched_ul_users(sched_ue_list& ue_db, sf_sched* tti_sched) override; // generate uplink users bitmask
    void record_cc_result(rbgmask_t cc_result) override;
    

private:
    void new_tti(sched_ue_list& ue_db, sf_sched* tti_sched); // check if the next tti is scheduled already, if not, call this function to schedule the next tti

    const sched_cell_params_t*  cc_cfg = nullptr; // stores the current cell parameters pass from the input
    float                       fairness_coeff = 1; 

    /* */

    
    
    

    srsran::tti_point current_tti_rx; //current tti

    struct ue_ctxt {
        ue_ctxt(uint16_t rnti_, uint8_t slice_, float fairness_coeff_) : rnti(rnti_), slice(slice_), fairness_coeff(fairness_coeff_) {}
        float    dl_avg_rate() const { return dl_nof_samples == 0 ? 0 : dl_avg_rate_; }
        float    ul_avg_rate() const { return ul_nof_samples == 0 ? 0 : ul_avg_rate_; }
        uint32_t dl_count() const { return dl_nof_samples; }
        uint32_t ul_count() const { return ul_nof_samples; }
        void     new_tti(const sched_cell_params_t& cell, sched_ue& ue, sf_sched* tti_sched);
        void     save_dl_alloc(uint32_t alloc_bytes, float alpha);
        void     save_ul_alloc(uint32_t alloc_bytes, float alpha);

        const uint16_t rnti;
        const float    fairness_coeff;
        const uint8_t  slice; 

        int                 ue_cc_idx  = 0;
        float               dl_prio    = 0;
        float               ul_prio    = 0;
        const dl_harq_proc* dl_retx_h  = nullptr;
        const dl_harq_proc* dl_newtx_h = nullptr;
        const ul_harq_proc* ul_h       = nullptr;
       

    private:
        float    dl_avg_rate_   = 0;
        float    ul_avg_rate_   = 0;
        uint32_t dl_nof_samples = 0;
        uint32_t ul_nof_samples = 0;
    }; 

    srsran::static_circular_map<uint16_t, ue_ctxt, SRSENB_MAX_UES> ue_history_db;

    struct ue_dl_prio_compare {
        bool operator()(const ue_ctxt* lhs, const ue_ctxt* rhs) const;
    };
    struct ue_ul_prio_compare {
        bool operator()(const ue_ctxt* lhs, const ue_ctxt* rhs) const;
    };     

    using ue_dl_queue_t = std::priority_queue<ue_ctxt*, std::vector<ue_ctxt*>, ue_dl_prio_compare>;
    using ue_ul_queue_t = std::priority_queue<ue_ctxt*, std::vector<ue_ctxt*>, ue_ul_prio_compare>;

    ue_dl_queue_t dl_queue;
    ue_ul_queue_t ul_queue;

    uint32_t try_dl_alloc(ue_ctxt& ue_ctxt, sched_ue& ue, sf_sched* tti_sched);
    uint32_t try_ul_alloc(ue_ctxt& ue_ctxt, sched_ue& ue, sf_sched* tti_sched);


    void set_reserved_carrier(uint32_t enb_cc_idx);


    uint32_t reserved_carrier = 0;
    bool cc_occupied = true;
    srsran::static_circular_map<uint32_t, float, 20> reserved_cc_occupied_result;

};
}

#endif // SRSRAN_SCHED_TIME_CR_H