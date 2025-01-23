# Copyright (c) 2019-2023 Huazhong University of Science and Technology, Dian Group
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Pengyu Liu <eic_lpy@hust.edu.cn>
#         Xiaojun Guo <guoxj@hust.edu.cn>
#         Hao Yin <haoyin@uw.edu>
#         Muyuan Shen <muyuan_shen@hust.edu.cn>


import numpy as np
import tensorflow as tf
import keras
from keras.layers import *
import sys
import gc
import keras.backend as K
import ns3ai_ltecqi_py as py_binding
from ns3ai_utils import Experiment
import traceback

# delta for prediction
delta = int(sys.argv[1])

MAX_RBG_NUM = 32


def new_print(filename="log", print_screen=False):
    old_print = print

    def print_fun(s):
        if print_screen:
            old_print(s)
        with open(filename, "a+") as f:
            f.write(s)
            f.write('\n')

    return print_fun


old_print = print
print = new_print(filename="log_" + str(delta), print_screen=False)

tf.random.set_seed(0)
np.random.seed(1)

input_len = 200
pred_len = 40
batch_size = 20
alpha = 0.6
not_train = False

lstm_input_vec = Input(shape=(input_len, 1), name="input_vec")
dense1 = Dense(30, activation='selu', kernel_regularizer='l1',)(
    lstm_input_vec[:, :, 0])
old_print(dense1)
lstm_l1_mse = tf.keras.ops.expand_dims(dense1, axis=-1)
lstm_mse = LSTM(20)(lstm_l1_mse)
predict_lstm_mse = Dense(1)(lstm_mse)
lstm_model_mse = keras.Model(inputs=lstm_input_vec, outputs=predict_lstm_mse)
lstm_model_mse.compile(optimizer="adam", loss="MSE")


def simple_MSE(y_pred, y_true):
    return (((y_pred - y_true)**2)).mean()


def weighted_MSE(y_pred, y_true):
    return (((y_pred - y_true)**2) * (1 + np.arange(len(y_pred))) /
            len(y_pred)).mean()


cqi_queue = []
prediction = []
last = []
right = []
corrected_predict = []
target = []
train_data = []
is_train = True
CQI = 0
delay_queue = []

exp = Experiment("ns3ai_ltecqi_msg", "../../../../../", py_binding, handleFinish=True)
msgInterface = exp.run(show_output=True)

try:
    while True:
        msgInterface.PyRecvBegin()
        if msgInterface.PyGetFinished():
            break
        gc.collect()
        # Get CQI
        CQI = msgInterface.GetCpp2PyStruct().wbCqi
        msgInterface.PyRecvEnd()

        if CQI > 15:
            break
        old_print("get: %d" % CQI)
        # CQI = next(get_CQI)
        delay_queue.append(CQI)
        if len(delay_queue) < delta:
            CQI = delay_queue[-1]
        else:
            CQI = delay_queue[-delta]
        if not_train:
            msgInterface.PySendBegin()
            msgInterface.GetPy2CppStruct().new_wbCqi = CQI
            msgInterface.PySendEnd()
            continue
        cqi_queue.append(CQI)
        if len(cqi_queue) >= input_len + delta:
            target.append(CQI)
        if len(cqi_queue) >= input_len:
            one_data = cqi_queue[-input_len:]
            train_data.append(one_data)
        else:
            msgInterface.PySendBegin()
            msgInterface.GetPy2CppStruct().new_wbCqi = CQI
            msgInterface.PySendEnd()
            old_print("set: %d" % CQI)
            continue

        data_to_pred = np.array(one_data).reshape(-1, input_len, 1) / 10
        _predict_cqi = lstm_model_mse.predict(data_to_pred)
        old_print(_predict_cqi)
        del data_to_pred
        prediction.append(int(_predict_cqi[0, 0] + 0.49995))
        last.append(one_data[-1])
        corrected_predict.append(int(_predict_cqi[0, 0] + 0.49995))
        del one_data
        if len(train_data) >= pred_len + delta:
            err_t = weighted_MSE(
                np.array(last[(-pred_len - delta):-delta]),
                np.array(target[-pred_len:]))
            err_p = weighted_MSE(
                np.array(prediction[(-pred_len - delta):-delta]),
                np.array(target[-pred_len:]))
            if err_p <= err_t * alpha:
                if err_t < 1e-6:
                    corrected_predict[-1] = last[-1]
                print(" ")
                print("OK %d %f %f" % ((len(cqi_queue)), err_t, err_p))
                right.append(1)
                pass
            else:
                corrected_predict[-1] = last[-1]
                if err_t <= 1e-6:
                    msgInterface.PySendBegin()
                    msgInterface.GetPy2CppStruct().new_wbCqi = CQI
                    msgInterface.PySendEnd()
                    print("set: %d" % CQI)
                    continue
                else:
                    print("train %d" % (len(cqi_queue)))
                    right.append(0)

                    lstm_model_mse.fit(x=np.array(
                        train_data[-delta - batch_size:-delta]).reshape(
                        batch_size, input_len, 1) / 10,
                                       y=np.array(target[-batch_size:]),
                                       batch_size=batch_size,
                                       epochs=1,
                                       verbose=0)
        else:
            corrected_predict[-1] = last[-1]
        # sm.Set(corrected_predict[-1])
        msgInterface.PySendBegin()
        msgInterface.GetPy2CppStruct().new_wbCqi = CQI
        msgInterface.PySendEnd()
        print("set: %d" % corrected_predict[-1])

except Exception as e:
    exc_type, exc_value, exc_traceback = sys.exc_info()
    print("Exception occurred: {}".format(e))
    print("Traceback:")
    traceback.print_tb(exc_traceback)
    exit(1)

else:
    with open("log_" + str(delta), "a+") as f:
        f.write("\n")
        if len(right):
            f.write("rate = %f %%\n" % (sum(right) / len(right)))
        f.write("MSE_T = %f %%\n" %
                (simple_MSE(np.array(target[delta:]), np.array(target[:-delta]))))
        f.write("MSE_p = %f %%\n" % (simple_MSE(
            np.array(corrected_predict[delta:]), np.array(target[:delta]))))

finally:
    print("Finally exiting...")
    del exp
