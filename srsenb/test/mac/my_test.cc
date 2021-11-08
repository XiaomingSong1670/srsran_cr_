/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "sched_test_common.h"
#include "sched_test_utils.h"
#include "srsenb/hdr/stack/mac/sched.h"
#include "srsran/common/common_lte.h"
#include "srsran/mac/pdu.h"

using namespace srsenb;

uint32_t seed = std::chrono::system_clock::now().time_since_epoch().count();

/*******************
 *     Logging     *
 *******************/

/// RAII style class that prints the test diagnostic info on destruction.
class sched_diagnostic_printer
{
public:
  explicit sched_diagnostic_printer(srsran::log_sink_spy& s) : s(s) {}

  ~sched_diagnostic_printer()
  {
    auto& logger = srslog::fetch_basic_logger("TEST");
    logger.info("[TESTER] Number of assertion warnings: %u", s.get_warning_counter());
    logger.info("[TESTER] Number of assertion errors: %u", s.get_error_counter());
    logger.info("[TESTER] This was the seed: %u", seed);
    srslog::flush();
  }

private:
  srsran::log_sink_spy& s;
};

/******************************
 *      Scheduler Tests
 *****************************/

sim_sched_args generate_default_sim_args(uint32_t nof_prb, uint32_t nof_ccs)
{
  sim_sched_args sim_args;

  // 2-4-1
  sim_args.default_ue_sim_cfg.ue_cfg = generate_default_ue_cfg2();

  // 2-4-2
  // std::vector<srsenb::sched_interface::cell_cfg_t> cell_cfg(nof_ccs, generate_default_cell_cfg(nof_prb));
  std::vector<srsenb::sched_interface::cell_cfg_t> cell_cfg;
  cell_cfg.push_back(generate_default_cell_cfg(15));
  cell_cfg.push_back(generate_default_cell_cfg(15));
  cell_cfg.push_back(generate_default_cell_cfg(15));
  cell_cfg[0].scell_list.resize(2);
  cell_cfg[0].scell_list[0].enb_cc_idx               = 1;
  cell_cfg[0].scell_list[0].cross_carrier_scheduling = false;
  cell_cfg[0].scell_list[0].ul_allowed               = true;
  cell_cfg[0].scell_list[1].enb_cc_idx               = 2;
  cell_cfg[0].scell_list[1].cross_carrier_scheduling = false;
  cell_cfg[0].scell_list[1].ul_allowed               = true;
  cell_cfg[1].cell.id                                = 2; 
  cell_cfg[1].scell_list                             = cell_cfg[0].scell_list;
  cell_cfg[1].scell_list[0].enb_cc_idx               = 0;
  cell_cfg[2].cell.id                                = 3;
  cell_cfg[2].scell_list.resize(1);
  cell_cfg[2].scell_list[0].enb_cc_idx               = 1;
  cell_cfg[2].scell_list[0].cross_carrier_scheduling = false;
  cell_cfg[2].scell_list[0].ul_allowed               = true;


  sim_args.cell_cfg                                  = std::move(cell_cfg);

  /* Setup Derived Params */
  sim_args.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(nof_ccs);
  for (uint32_t i = 0; i < sim_args.default_ue_sim_cfg.ue_cfg.supported_cc_list.size(); ++i) {
    sim_args.default_ue_sim_cfg.ue_cfg.supported_cc_list[i].active     = true;
    sim_args.default_ue_sim_cfg.ue_cfg.supported_cc_list[i].enb_cc_idx = i;
  }

  return sim_args;
}


struct test_scell_activation_params {
  uint32_t pcell_idx = 0;
};

int test_scell_activation(uint32_t sim_number, test_scell_activation_params params)
{
  uint32_t nof_prb   = 15;
  uint32_t nof_ccs   = 3;
  uint32_t start_tti = 0; 
  float          ul_sr_exps[]   = {1, 4}; // log rand
  float          dl_data_exps[] = {1, 4}; // log rand
  float          P_ul_sr = randf() * 0.5, P_dl = randf() * 0.5;
  const uint16_t rnti1 = 70;
  const uint16_t rnti2 = 71;
  const uint16_t rnti3 = 72;
  const uint16_t rnti4 = 73;
  const uint16_t rnti5 = 74;
  const uint16_t rnti6 = 75;
  uint32_t prach_tti = 1;
  uint32_t msg4_size = 40;
  uint32_t duration  = 1000;
  std::vector<uint32_t> cc_idxs(nof_ccs);
  std::iota(cc_idxs.begin(), cc_idxs.end(), 0);
  std::shuffle(cc_idxs.begin(), cc_idxs.end(), get_rand_gen());
//  std::iter_swap(cc_idxs.begin(), std::find(cc_idxs.begin(), cc_idxs.end(), params.pcell_idx));

  // 2-4
  /* Setup simulation arguments struct */
  sim_sched_args ue1 = generate_default_sim_args(nof_prb, nof_ccs);
  sim_sched_args ue2 = generate_default_sim_args(nof_prb, nof_ccs);
  sim_sched_args ue3 = generate_default_sim_args(nof_prb, nof_ccs);
  sim_sched_args ue4 = generate_default_sim_args(nof_prb, nof_ccs);
  sim_sched_args ue5 = generate_default_sim_args(nof_prb, nof_ccs);
  sim_sched_args ue6 = generate_default_sim_args(nof_prb, nof_ccs);

  ue1.start_tti      = start_tti;
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(3);
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[0];
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[1];
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[2].enb_cc_idx                            = cc_idxs[2];
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue1.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;
 ue1.sched_args.sched_policy                                                              = "time_cr";
  ue1.sched_args.slice                                                                     = 1;
  ue1.default_ue_sim_cfg.ue_cfg.slice                                                      = 1;

  ue2.start_tti      = start_tti;
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(3);
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[0];
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[1];
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[2].enb_cc_idx                            = cc_idxs[2];
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue2.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;
 ue2.sched_args.sched_policy                                                              = "time_cr";
  ue2.sched_args.slice                                                                     = 1;
  ue2.default_ue_sim_cfg.ue_cfg.slice                                                      = 1;

  ue3.start_tti      = start_tti;
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(3);
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[0];
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[1];
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[2].enb_cc_idx                            = cc_idxs[2];
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue3.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;
 ue3.sched_args.sched_policy                                                              = "time_cr";
  ue3.sched_args.slice                                                                     = 2;
  ue3.default_ue_sim_cfg.ue_cfg.slice                                                      = 2;

  ue4.start_tti      = start_tti;
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(3);
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[0];
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[1];
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[2].enb_cc_idx                            = cc_idxs[2];
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue4.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;
 ue4.sched_args.sched_policy                                                              = "time_cr";
  ue4.sched_args.slice                                                                     = 2;
  ue4.default_ue_sim_cfg.ue_cfg.slice                                                      = 2;

  ue5.start_tti      = start_tti;
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(2);
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[1];
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[2];
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue5.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;
 ue5.sched_args.sched_policy                                                              = "time_cr";
  ue5.sched_args.slice                                                                     = 3;
  ue5.default_ue_sim_cfg.ue_cfg.slice                                                      = 3;

  ue6.start_tti      = start_tti;
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list.resize(2);
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].active                                = true;
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].enb_cc_idx                            = cc_idxs[1];
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list[1].enb_cc_idx                            = cc_idxs[2];
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.periodic_configured = true;
  ue6.default_ue_sim_cfg.ue_cfg.supported_cc_list[0].dl_cfg.cqi_report.pmi_idx             = 37;  
 ue6.sched_args.sched_policy                                                              = "time_cr";
  ue6.sched_args.slice                                                                     = 3;
  ue6.default_ue_sim_cfg.ue_cfg.slice                                                      = 3;

  // 2-6
  /* Simulation Objects Setup */
  sched_sim_event_generator generator;
  // Setup scheduler
  common_sched_tester tester;
  tester.sim_cfg(ue1);


  /* Simulation */

  // Event PRACH: PRACH takes place for "rnti1", and carrier "pcell_idx"
  generator.step_until(1);
  tti_ev::user_cfg_ev* user1 = generator.add_new_default_user(duration, ue1.default_ue_sim_cfg);
  user1->rnti                = rnti1;

  generator.step_until(10);
  tti_ev::user_cfg_ev* user2 = generator.add_new_default_user(duration, ue2.default_ue_sim_cfg);
  user2->rnti                = rnti2;

  generator.step_until(20);
  tti_ev::user_cfg_ev* user3 = generator.add_new_default_user(duration, ue3.default_ue_sim_cfg);  
  user3->rnti                = rnti3;

  generator.step_until(30);
  tti_ev::user_cfg_ev* user4 = generator.add_new_default_user(duration, ue4.default_ue_sim_cfg);
  user4->rnti                = rnti4;

  generator.step_until(40);
  tti_ev::user_cfg_ev* user5 = generator.add_new_default_user(duration, ue5.default_ue_sim_cfg);
  user5->rnti                = rnti5;

  generator.step_until(50);
  tti_ev::user_cfg_ev* user6 = generator.add_new_default_user(duration, ue6.default_ue_sim_cfg);
  user6->rnti                = rnti6;
  generator.step_until(60);
  tester.test_next_ttis(generator.tti_events);

  TESTASSERT(tester.sched_sim->user_exists(rnti1));
  TESTASSERT(tester.sched_sim->user_exists(rnti2));
  TESTASSERT(tester.sched_sim->user_exists(rnti3));
  TESTASSERT(tester.sched_sim->user_exists(rnti4));
  TESTASSERT(tester.sched_sim->user_exists(rnti5));
  TESTASSERT(tester.sched_sim->user_exists(rnti6));

  uint32_t cqi = 14;
  for (uint32_t i = 1; i < cc_idxs.size(); ++i) {
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti1, cc_idxs[i], cqi);
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti2, cc_idxs[i], cqi);
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti3, cc_idxs[i], cqi);
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti4, cc_idxs[i], cqi);
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti5, cc_idxs[i], cqi);
    tester.dl_cqi_info(tester.tti_rx.to_uint(), rnti6, cc_idxs[i], cqi);
  }
  generator.step_until(100);
  auto test_data_burst = [&]() {

    generator.add_dl_data(rnti5, 1000000);
    generator.add_dl_data(rnti6, 1000000);
    for (uint32_t i = 0; i < 500; ++i) {


      if (i % 10 == 0) {
          generator.add_dl_data(rnti1, 100+20*i);
//          generator.add_dl_data(rnti2, 100+10*i);
          generator.add_dl_data(rnti4, 100+20*i);
//          generator.add_dl_data(rnti4, 100+10*i);
//          generator.add_dl_data(rnti5, 100+10*i);
//          generator.add_dl_data(rnti6, 100+10*i);
      }
      generator.step_tti();
    };
  };
  test_data_burst();



  generator.step_until(600);
  tester.test_next_ttis(generator.tti_events);
  // generator.step_tti();
  int a = 1;

  srslog::flush();
  printf("[TESTER] Sim%d finished successfully\n\n", sim_number);
  return SRSRAN_SUCCESS;
}

int main()
{
  // Setup rand seed
  set_randseed(seed);

  // Setup the log spy to intercept error and warning log entries.
  if (!srslog::install_custom_sink(
          srsran::log_sink_spy::name(),
          std::unique_ptr<srsran::log_sink_spy>(new srsran::log_sink_spy(srslog::get_default_log_formatter())))) {
    return SRSRAN_ERROR;
  }

  auto* spy = static_cast<srsran::log_sink_spy*>(srslog::find_sink(srsran::log_sink_spy::name()));
  if (!spy) {
    return SRSRAN_ERROR;
  }

  auto& mac_log = srslog::fetch_basic_logger("MAC");
  mac_log.set_level(srslog::basic_levels::debug);
  auto& test_log = srslog::fetch_basic_logger("TEST", *spy, false);
  test_log.set_level(srslog::basic_levels::debug);

  // Start the log backend.
  srslog::init();

  sched_diagnostic_printer printer(*spy);

  printf("[TESTER] This is the chosen seed: %u\n", seed);
  uint32_t N_runs = 20;
  for (uint32_t n = 0; n < N_runs; ++n) {
    printf("[TESTER] Sim run number: %u\n", n);

    test_scell_activation_params p = {};
    p.pcell_idx                    = 0;
    TESTASSERT(test_scell_activation(n * 2, p) == SRSRAN_SUCCESS);

    p           = {};
    p.pcell_idx = 1;
    TESTASSERT(test_scell_activation(n * 2 + 1, p) == SRSRAN_SUCCESS);
  }

  srslog::flush();

  return 0;
}
