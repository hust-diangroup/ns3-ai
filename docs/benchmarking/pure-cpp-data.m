% Draw figures for pure C++ (libtorch) benchmarking

% CPU cycles during RL algorithm processing (from states to actions)
% Left: RL-TCP using struct-based message interface (PyTorch)
% Right: RL-TCP using pure C++ library (libtorch)

seed = [0, 1, 2, 3, 4, 5];
cycle_mean = [
    745949 277715;
    731465 277458;
    732394 274952;
    732369 284828;
    730028 281694;
    730009 281241;
    ];
cycle_stddev = [
    113250 14796.1;
    110334 13034.3;
    112710 12033;
    112047 17423.6;
    110129 15625;
    110765 12591.3;
    ];

figure;
h = bar(seed, cycle_mean);
errorbar_x = zeros(6, 2);
for i = 1 : 2
    errorbar_x(:, i) = h(1, i).XEndPoints';
end
hold on;
errorbar(errorbar_x, cycle_mean, cycle_stddev, 'LineStyle', 'none');
hold off;
ylim([0, 1e6]);
legend({'msg interface', 'pure c++'});
title('Processing Time');
xlabel('Test Case (simulation run)');
ylabel('CPU Cycles')
