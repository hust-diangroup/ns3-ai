# AUTHOR(S):
# Mattia Lecci <mattia.lecci@dei.unipd.it>
#
# University of Padova (UNIPD), Italy
# Information Engineering Department (DEI)
# SIGNET Research Group @ http://signet.dei.unipd.it/
#
# Date: December 2020

import sem
import os
import sys
import argparse
from collections import OrderedDict
import numpy as np
from matplotlib import pyplot as plt
import copy
import tikzplotlib

sys.stdout.flush()

# SEM script that generates the plots of the reference paper.
# For further information please check the reference paper (see README.md).

def remove_simulations(broken_results):
    print("Removing broken simulations")
    # TODO test map(campaign.db.delete_result, broken_results.flatten())
    for result in broken_results.flatten():
        if result:
            print("removing ", str(result['meta']['id']))
            campaign.db.delete_result(result)
    # write updated database to disk
    campaign.db.write_to_disk()


def check_stderr(result):
    if len(result['output']['stderr']) > 0:
        print('Invalid simulation: ', result['meta']['id'], file=sys.stderr)
        return result
    else:
        return []


def compute_avg_burst_thr_mbps(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['burstTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize
    delays = np.array([float(row.split(',')[4]) * 8 / 1e6 # Mb
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.sum(delays) / results['params']['simulationTime']
    else:
        delay = 0

    # print(delay)

    return delay


def compute_avg_burst_delay_ms(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['burstTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize
    delays = np.array([(float(row.split(',')[2]) - float(row.split(',')[1])) / 1e6 # ms
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.mean(delays)
    else:
        delay = 0

    # print(delay)

    return delay


def compute_95perc_burst_delay_ms(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['burstTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize
    delays = np.array([(float(row.split(',')[2]) - float(row.split(',')[1])) / 1e6 # ms
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.percentile(delays, 95)
    else:
        delay = 0

    # print(delay)

    return delay


def compute_burst_succ_rate(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['txBurstsBySta.csv']
    tot_tx = np.sum([float(n) for n in trace.split('\n')[:-1]])

    trace = results['output']['rxBursts.csv']
    tot_rx = float(trace.rstrip('\n'))


    if tot_rx > 0:
        succ = tot_rx / tot_tx
    else:
        succ = 0

    return succ


def compute_avg_fragment_thr_mbps(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['fragmentTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,FragSeq,TotFrags,FragSize
    delays = np.array([float(row.split(',')[-1]) * 8 / 1e6 # Mb
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.sum(delays) / results['params']['simulationTime']
    else:
        delay = 0

    # print(delay)

    return delay


def compute_avg_fragment_delay_ms(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['fragmentTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize
    delays = np.array([(float(row.split(',')[2]) - float(row.split(',')[1])) / 1e6 # ms
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.mean(delays)
    else:
        delay = 0

    # print(delay)

    return delay


def compute_95perc_fragment_delay_ms(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['fragmentTrace.csv']

    # SrcAddress,TxTime_ns,RxTime_ns,BurstSeq,BurstSize
    delays = np.array([(float(row.split(',')[2]) - float(row.split(',')[1])) / 1e6 # ms
                      for i, row in enumerate(trace.split('\n')[:-1]) if (i > 0)])

    if len(delays) > 0:
        delay = np.percentile(delays, 95)
    else:
        delay = 0

    # print(delay)

    return delay


def compute_fragment_succ_rate(results):
    # print("id:", results['meta']['id'])
    trace = results['output']['txFragmentsBySta.csv']
    tot_tx = np.sum([float(n) for n in trace.split('\n')[:-1]])

    trace = results['output']['rxFragments.csv']
    tot_rx = float(trace.rstrip('\n'))

    if tot_rx > 0:
        succ = tot_rx / tot_tx
    else:
        succ = 0

    return succ


def plot_line_metric(campaign, parameter_space, result_parsing_function, runs, xx, hue_var, xlabel, ylabel, filename, ylim=None, xscale="linear", yscale="linear"):
    print("Plotting (line): ", os.path.join(img_dir, filename))
    metric = campaign.get_results_as_xarray(parameter_space,
                                            result_parsing_function,
                                            xlabel,
                                            runs)
    # average over numRuns and squeeze
    metric_mean = metric.reduce(np.mean, 'runs').squeeze()
    metric_ci95 = metric.reduce(np.std, 'runs').squeeze() * 1.96 / np.sqrt(runs)

    fig = plt.figure()
    for val in metric_mean.coords[hue_var].values:
        plt.errorbar(xx, metric_mean.sel({hue_var: val}),
                     yerr=metric_ci95.sel({hue_var: val}),
                     label=f"{hue_var}={val}")
    plt.xscale(xscale)
    plt.yscale(yscale)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.ylim(ylim)
    plt.legend()
    plt.grid()
    fig.savefig(os.path.join(img_dir, filename + ".png"))
    try:
        tikzplotlib.save(os.path.join(img_dir, filename + ".tex"))
    except Exception as e:
        print("Did not convert to tikz, error occurred: ", e)
    plt.close(fig)


###############
# Main script #
###############
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--cores",
                        help="Value of sem.parallelrunner.MAX_PARALLEL_PROCESSES. Default: 1",
                        type=int,
                        default=1)
    parser.add_argument("--numRuns",
                        help="The number of runs per simulation. Default: 50",
                        type=int,
                        default=50)
    parser.add_argument("--campaignName",
                        help="MANDATORY parameter for the campaign name. Suggested: commit hash",
                        default=None)
    parser.add_argument("--paramSet",
                        help="MANDATORY parameter set",
                        default=None)
    parser.add_argument("--frameRate",
                        help="Frame rate. Default: 60",
                        default=60)
    args = parser.parse_args()

    assert args.campaignName is not None, "Undefined parameter --campaignName"
    assert args.paramSet is not None, "Undefined parameter --paramSet"
    print(f'Starting sem simulation with {args.cores} core(s)...')

    sem.parallelrunner.MAX_PARALLEL_PROCESSES = args.cores
    ns_path = os.path.dirname(os.path.realpath(__file__))
    campaign_name = args.campaignName
    script = "vr-app-n-stas"
    campaign_dir = os.path.join(ns_path, "campaigns", f"{campaign_name}-{args.paramSet}")
    img_dir = os.path.join(ns_path, 'campaigns-img', f"{campaign_name}-{args.paramSet}")

    # Set up campaign
    campaign = sem.CampaignManager.new(
        ns_path, script, campaign_dir,
        overwrite=False,
        runner_type="ParallelRunner",
        optimized=True,
        check_repo=False
    )

    print("campaign: " + str(campaign))

    # Need to create the img/ folder after setting up a new campaign
    os.makedirs(img_dir, exist_ok=True)

    # Set up baseline parameters
    if args.paramSet == "nStas":
        param_combination = OrderedDict({
            "appRate": "50Mbps",
            "frameRate": args.frameRate,
            "burstGeneratorType": ["model", "trace", "deterministic"],
            "nStas": list(range(1, 8+1)),
            "simulationTime": 10,
            "RngRun": list(range(args.numRuns))
        })

    elif args.paramSet == "appRate":
        param_combination = OrderedDict({
            "appRate": [f"{rate}Mbps" for rate in range(10, 50+1, 10)],
            "frameRate": args.frameRate,
            "burstGeneratorType": ["model", "trace", "deterministic"],
            "nStas": 1,
            "simulationTime": 10,
            "RngRun": list(range(args.numRuns))
        })
    else:
        raise ValueError(f"paramSet={args.paramSet} not recognized")

    campaign.run_missing_simulations(param_combination)

    # try to fix broken results
    broken_results = campaign.get_results_as_numpy_array(param_combination,
                                                         check_stderr,
                                                         args.numRuns)
    # remove_simulations(broken_results)
    # campaign.run_missing_simulations(param_combination)

    # remove RngRun: in contrast with get results
    param_combination.pop("RngRun")

    # Plots
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_burst_thr_mbps,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Avg. Burst Throughput [Mbps]',
                     filename='avg_burst_thr_mbps')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_burst_delay_ms,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Avg. Burst Delay [ms]',
                     filename='avg_burst_delay_ms')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_95perc_burst_delay_ms,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='95th perc. Burst Delay [ms]',
                     filename='95perc_burst_delay_ms')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_burst_succ_rate,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Burst Succ. Rate',
                     filename='burst_succ_rate')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_fragment_thr_mbps,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Avg. Fragment Throughput [Mbps]',
                     filename='avg_fragment_thr_mbps')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_fragment_delay_ms,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Avg. Fragment Delay [ms]',
                     filename='avg_fragment_delay_ms')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_95perc_fragment_delay_ms,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='95th perc. Fragment Delay [ms]',
                     filename='95perc_fragment_delay_ms')

    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_fragment_succ_rate,
                     runs=args.numRuns,
                     xx=param_combination[args.paramSet],
                     hue_var="burstGeneratorType",
                     xlabel=args.paramSet,
                     ylabel='Fragment Succ. Rate',
                     filename='fragment_succ_rate')

