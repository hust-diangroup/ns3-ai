# CQI Prediction
This original work is done based on [5G NR](https://5g-lena.cttc.es/) branch in ns-3. We made some changes to make it also run in LTE codebase in ns-3 mainline. We didn't reproduce all the experiments on LTE, and the results used in this document are based on NR work.

Unlike the RL_TCP example, in this example, we want to show how to change the source code to use our ns3-ai model. 
## Objective
- The fast time-varying channel strictly limits the throughput performance of the 5G system. 
- In high mobility scenes, the performance of the AMC system is significantly deteriorated, resulting from rapidly time-varying channels. 
- Reliable channel prediction to forecast the channel variation is necessary.
- CQI also has a great impact on the MAC Scheduler results, and its generation and transmission delay will cause inaccuracy in BS.


## CQI prediction simulation in NS3 


### Simulation scenario
This scenario is implemented to test the performance of high-speed situations, and multi-users are attached to the base station to test the downlink scheduling performance.
<img src="figures/scene1.png" alt="scenario" width="200"/>


### Performance Metrics
- Calculate the MSE of the outdated CQI and predicted CQI
- Compare the MSE, calculate the rate of the prediction (usage rate)
- Using Radom Robin as a scheduler to see directly the impact of CQI (Every user has an equal number of times scheduled )
- Mainly concern throughput





### Simulation process：

- The user reports the CQI to the BS. 
- BS send the CQI to the online training module.
- Using LSTM to predict the CQI and shipping back to ns-3. 
- Using the feedback value for the next scheduling decision.



## DL Algorithms Using for Prediction

- Neural Networks(NN): Easy, fast, and was used by the above works.
- Long Short-Term Memory (LSTM) is ideal for dealing with issues that are highly correlated with time series.
- The changing of CQI in the BS side can be evaluated and predicted by the past series of CQI. 



## Interaction between DL and ns-3


- Set up the environment

```
ns-3:
env = Create<TcpTimeStepShmEnv> (1234);//1234 shared-memory key


python：
py_interface.Init(1234, 4096)#pool size = 4096
var = py_interface.ShmBigVar(1234, TcpRl)
```



- In ns-3, put CQI into shared memory for use by the DL algorithm.

ns-3 side：

```cpp
uint8_t newCqi = params.m_cqiList.at (i).m_wbCqi.at (0);
NS_ASSERT_MSG (m_cqiDl != NULL, "DL env error");
if (rnti == 1)
{
 uint8_t oldCqi = newCqi;
 m_cqiDl->SetWbCQI (newCqi);
 newCqi = m_cqiDl->GetWbCQI ();
 std::cout<<"At: "<<Simulator::Now().GetSeconds()<<"s CQI: "<<(int)oldCqi<<"->"<<(int)newCqi<<std::endl;
}
```



- Python trains based on the read data to use LSTM to predict the CQI.

python side：

```python
with dl as data:
 if data == None:
 break
 # print('data.env.wbCqi', data.env.wbCqi)
 # Deep Learning code there
 data.act.new_wbCqi = data.env.wbCqi
 data.act.new_sbCqi = data.env.sbCqi
```



## Build and Run
Check and Intall required packets for the tensorflow:
```shell
pip3 install -r requirements.txt
```
### Run Separately (no longer recommended)
Run ns-3 example with two shell window:
```shell
cp -r contrib/ns3-ai/example/lte_cqi scratch/
./waf --run "lte_cqi"
```
Open another shell window and Run Python code:
```shell
cd scratch/lte_cqi/

python3 run_online.py
```
**NOTICE: ns3 code and Python code need to run simultaneously**

### Run with all-in-one script
If you want to test the LSTM, you can run another python script but you may need to install [TensorFlow](https://www.tensorflow.org/) environment first. 
```shell
cd scratch/lte_cqi/

python3 run_online_lstm.py 1
```    




## Results
These results are based on the NR code, not the LTE code. Currently, the "run_online.py" script does nothing but exchanges data, so you don't need to set up DL environments.

Using the data generated from ns-3, we test the usage rate of different DL algorithms. Two different prediction methods are applied as the prediction module, the FNN used by another group and LSTM. Form the figure, as the speed improved, the CQI is more difficult to predict and the usage is down. The LSTM module is much more reliable than the FNN module, which can have a usage above 50%. As the speed is increasing, the usage is decreasing, and the FNN could not provide a reliable prediction, but LSTM is still working.
![Accuracy](figures/accuracy_less.png)
![Throughput](figures/throughput.png)
