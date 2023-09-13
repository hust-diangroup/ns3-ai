% Draw figures for Gym interface benchmarking

% CPU cycles during interprocess communication
% Left: ns3-gym
% Right: ns3-ai's Gym interface

seed = [0, 1, 2, 3, 4, 5];
cpp2py_mean = [
    115605 6288;
    116412 7584;
    116335 5645;
    113930 6129;
    125322 6550;
    114498 6911;
    ];
cpp2py_stddev = [
    17122.9 2392.99;
    18810.3 1953.11;
    18649.2 3220.74;
    15256.4 2335.87;
    16987.4 2499.89;
    14800.4 2517.83;
    ];
py2cpp_mean = [
    126076 7665;
    126901 7599;
    127789 7235;
    122230 7470;
    133081 7723;
    126814 7810;
];
py2cpp_stddev = [
    12149.7 1106.71;
    12016.5 1225.58;
    13374.7 1111.01;
    15771.1 1025.5;
    13282.7 1122.41;
    11829.3 586.296;
];

% C++ to Python

figure;
h = bar(seed, cpp2py_mean);
errorbar_x = zeros(6, 2);
for i = 1 : 2
    errorbar_x(:, i) = h(1, i).XEndPoints';
end
hold on;
errorbar(errorbar_x, cpp2py_mean, cpp2py_stddev, 'LineStyle', 'none');
hold off;
ylim([0, 15e4]);
legend({'ns3-gym', 'ns3-ai'});
title('C++ to Python Transmission Time');
xlabel('Test Case (seed number)');
ylabel('CPU Cycles')

% Python to C++

figure;
h = bar(seed, py2cpp_mean);
errorbar_x = zeros(6, 2);
for i = 1 : 2
    errorbar_x(:, i) = h(1, i).XEndPoints';
end
hold on;
errorbar(errorbar_x, py2cpp_mean, py2cpp_stddev, 'LineStyle', 'none');
hold off;
ylim([0, 15e4]);
legend({'ns3-gym', 'ns3-ai'});
title('Python to C++ Transmission Time');
xlabel('Test Case (seed number)');
ylabel('CPU Cycles')
