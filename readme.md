# Hydrocontrol

Homemade hydroponic control system.

## Getting Started

Started with print http://www.thingiverse.com/thing:3601354 , developed light handlers, simpler cups, this controller, controller box.

### Hardware

* Microcontroller : STM32 F103C8T6
* Screen          : LCD 1602A with I2C adapter
* Relay           : 2x 10A 250V coil relay module 
* Encoder         : encoder module
* Power           : 5v DC


## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

**Urgor** - [urgorka@gmai.com](urgorka@gmail.com)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* [STMicroelectronics](https://www.st.com/)
* [Techmaker](https://techmaker.ua/)
* My wife Kate for patience for all this crazyness

## Menu
```
1234567890123456+
                |
Water day    [1]|
07:00-22:00     |
                |
Light day    [2]|
07:00-22:00     |
                |
Current time [3]|
12:30           |
                |
Water mode   [4]|
 on  off +timer |
                |
Light mode   [5]|
 on  off +timer |
================+
```
## Pinout
* PB6 - I2C SCL
* PB7 - I2C SDA
* PA6 - Encoder TIM3CH1	timer mode 
* PA7 - Encoder TIM3CH2	timer mode
* PB12 - Encoder Switch	interruption mode
* PB10 - Relay water		depends on TIM4 counter mode
* PB11 - Relay light		depends on TIM4 counter mode
* PB1 - Screen backlight	depends on TIM4 counter mode

## State mashine
```
() - signal
[] - additional condition
<> - additional action

+===================+                   +==================+
| MENU_MODE_LISTING |                   | MENU_MODE_TUNING | <--+
+===================+                   +==================+    |
        ^         |-----------------------^      |       |------+ (click) [idx<maxIdx]
        |          (click)                       |                <idx++>
        |          <idx=0>                       |
        |                                        |
        +----------------------------------------+
         (click) [idx=max]          
                   
```