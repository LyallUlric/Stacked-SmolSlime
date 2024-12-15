# Tracker firmware for SlimeNRF

This repository adds support to stack the IMU on top of the NRF 52840 pro micro board using the gpio pins to power instead of VCC. Default configuration has 3.3v connect ot pin 17, gnd to pin 20, scl to pin 22, and sda to pin 24, int to pin 2, and ext-clk to pin 29. Sleep mode will only work when both pin 17 and VCC are connected unless you are using ICM45 where you would connect BOTH int and ext-clk. As such, sleep is disabled by default and you don't need to connect int or ext-clk for icm if you aren't going to use it.


## Hardware
- https://github.com/SlimeVR/SlimeVR-Tracker-nRF-PCB
- https://oshwlab.com/sctanf/slimenrf3

## License
Unless otherwise specified, all code in this repository is dual-licensed under either:

- MIT License ([LICENSE-MIT](LICENSE-MIT) or https://opensource.org/license/mit/)
- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or https://opensource.org/license/apache-2-0/)

at your option. This means you can select the license you prefer!

Unless you explicitly state otherwise, any contribution intentionally submitted for
inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual
licensed as above, without any additional terms or conditions.
