%% detailed_data_acquisition.m
% triggers ultrasonic recording
% receives the data
% plots distances along with processing steps
clear;
close all;
addpath("plot scripts\");

%% Parameters
ITERATIONS = 200; 
MIC_NUM = 8;
MIC_NAMES = {"MIC 1", "MIC 2","MIC 3", "MIC 4", "MIC 5", "MIC 6","MIC 7", "MIC 8"};
DATA_LENGTH = 32 * 32;
PROCESSING_STEPS = 3; % raw, fitlered, xcorr

%% Arduino Setup + Config
% Serial port configuration 
ARDUINO_PORT = 'COM20';
ARDUINO_BAUDRATE = 115200;
arduino = serialport(ARDUINO_PORT, ARDUINO_BAUDRATE); % select port and baudrate 

%% Arrays
dist_matrix = zeros(MIC_NUM, ITERATIONS); % distance matrix
processing_matrix = zeros(ITERATIONS, MIC_NUM, PROCESSING_STEPS, DATA_LENGTH); % all processing steps data
processing_matrix_size = size(processing_matrix);
time_axis = zeros(1, ITERATIONS); %  time array

%% Readings Loop
pause(1);
tic;
for it = 1:ITERATIONS
    write(arduino, 't', "char"); % trigger arduino measurement
    time_axis(it) = toc;
    dist_matrix(:, it) = read_distance_data(arduino, MIC_NUM);
end
acquisition_time = toc;

% save measurements
if ~exist("Measurements", 'dir')
    mkdir("Measurements");
end
file_name = sprintf('%s_%s.mat', "Measurements/dataset", datetime("now"));
file_name = strrep(file_name, ' ', '_');
file_name = strrep(file_name, ':', '-');
save(file_name, "dist_matrix", "time_axis");

fprintf("Data acquisition completed in: %fsec\n", acquisition_time);
fprintf("Average time between measurements: %fsec\n", acquisition_time/ITERATIONS);

% close serial connection
arduino = [];

%% Plotting 1
figure
for i = 1:MIC_NUM
    switch i
        case 1
            m = "o";
        case 2
            m = "^";
        case 3
            m = "square";
        case 4
            m = "diamond";
        case 5
            m = "v";
        case 6
            m = "hexagram";
        case 7
            m = "pentagram";
        case 8
            m = ">";
    end

    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2, 'Marker', m); hold on;

end
ylim([0 1])
xlim([0 time_axis(end)])
grid on
xlabel("time [s]");
ylabel("distance [m]")
legend(MIC_NAMES);
title("Microphone distance measurements")
beautify_plot(gcf, 1);

%% Plotting 2
figure
for i = 1:MIC_NUM
    subplot(MIC_NUM, 1, i);
    plot(time_axis, dist_matrix(i, :), 'LineWidth', 2)
    ylim([0 1])
    xlim([0 time_axis(end)])
    grid on
    xlabel("time [s]");
    ylabel("distance [m]")
    title(MIC_NAMES(i));
end
beautify_plot(gcf, 1);


function dist_vector = read_distance_data(arduino, mic_num)
    dist_vector = zeros(mic_num, 1);
    for i = 1:mic_num
        serial_rx_data = read(arduino, 4, 'uint8'); % 32bit per one distance measurement
        dist_vector(i, 1) = double(typecast(uint8(serial_rx_data), 'uint32'))/1e6; % expected in micrometers
    end
end
