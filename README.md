# srsran_cr_

srsran_cr proposed a QoS awared MAC scheduler, logically slice the network based on the carrier frequency. It creates three slices for three different types of SG Applications. Slice 1 is designed for controls signals, with high reliability and bounded-latency. Slice 2 is designed for real-time applications with guaranteed bit rate. Slice 3 is designed for period logs or large file transfers. Slice 3 can utilise all the resources of the network, but has no bit-rate of latency guaranteed. 


1. The proposed scheduling algorithm is at srsenb/src/stack/mac/schedulers/sched_time_cr.cc 

2. In order to use this algorithm

	1) The Carrier Aggregation should be enabled at ENB, at least 3 Cells must be configured at rr.conf
	2) In HSS (user_db.csv), the QCI for slice 1 is QCI = 1, the QCI for slice 2 QCI = 2, and the QCI for slice 3 is QCI = 3. In real deployment scenario, their values are supposed to be 85, 86, and 87; however, these values are not implemented in srsRAN (see 3GPP standards for more details about SG QCI). 
	3) Select this algorithm by adding 	--scheduler.policy="time-cr" 	when running the enb. Otherwise, you can configure this algorithms from the enb configuration file. 

3. The reservation level for the reserved carrier is set inexplicitly. The carrier manager may not be able to correctly monitor the carrier. More commitment is required for this part. 

4. A tester is implemented for delay and throughput test. It can simulate LTE DL and UL procedure below the MAC layer (more precisely, data from logical channels to UEs). It can also emulate the channel fading by setting arbitrary CQI at differet TTI. 

More information please contact the author Xiaoming Song via sxm1670@gmail.com

