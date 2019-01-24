idlearray = csvread("idle_r=8.csv");
busyarray = csvread("busy_r=8.csv");
collisionarray = csvread("collision_r=8.csv");
busyarray1 = csvread("busy_l=20.csv");
busyarray2 = csvread("busy_l=40.csv");
busyarray4 = csvread("busy_l=60.csv");
busyarray8 = csvread("busy_l=80.csv");
busyarray16 = csvread("busy_l=100.csv");


x = 5:5:500;

hold
% 
plot(x, busyarray1);
plot(x, busyarray2);
plot(x, busyarray4);
plot(x, busyarray8);
plot(x, busyarray16);

% plot(x, collisionarray);
% plot(x, busyarray);
% plot(x, idlearray);