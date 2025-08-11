---
title: Ultrasonic Ranging WiFi GUI
layout: default
math: mathjax
parent: Projects
nav_order: 2
---


# Ultrasonic Ranging WiFi GUI
{: .no_toc .fs-8 .fw-500}
---

{: .WARNING}
This project is currently in its initial development phase and is subject to significant changes, updates, and improvements.

- TOC
{:toc}


## Introduction

For the whole ultrasonic implementation info etc, refer to main [Ultrasonic Ranging page]({% link Projects/UltraSound.md %}). This page contains only difference showcase: WiFi implementation and GUI.

## Ideas
* For simplicity, use just one channel
* Button to start/stop continuous measurement
* Button to do x1 one shot measurement
* Button to switch between detailed data transmission or only distances
* Tabs to switch between views: distance only like real time running, then raw data, filtered, xcorr
* If only distances selected, tabs related to raw data, filtered etc. are grayed out

Rough sketch (you can change it a lot according to needs):
![alt text]({{site.baseurl}}/assets/images/gui_sketch.png)

Old one as a reference:
![alt text]({{site.baseurl}}/assets/images/gui_old.png)