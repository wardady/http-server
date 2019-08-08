# :computer: SoftServer :computer:
> minimalistic http-server for static pages

[![forthebadge](https://forthebadge.com/images/badges/made-with-c-plus-plus.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/built-with-love.svg)](https://forthebadge.com)

[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)](http://makeapullrequest.com)

## Table of contents
 - [Getting started](#getting-started)
   - [Prerequisites](#prerequisites)
   - [Installing](#installing)
   - [Usage](#usage)
 - [Contributing](#contributing)
 - [Team](#team)
 - [License and Copyright](license-and-copyright)

## Getting started

### Prerequisites
  For this project `qt` and `boost` libraries are needed :neckbeard:

### Installing
1. Pre-requirements: 
   - Boost 1.70, 
   - Qt5, 
   - CMake 3.10 or newer.   
    
2. Clone project.
3. Compile project.
     ```bash
     mkdir build && cd build
     cmake -G"Unix Makefiles" ..
     make
     ```
4. Run async with wanted IP-adress, port, path to a served documents and a number of threads.
     
     Example: `async 127.0.0.1 8080 /srv/http 4`
5. Enjoy! :tada:

Boost 1.66-1.69 may work too - not tested. 1.65 and older definitely uncompatible. 
Regarding updating boost on Ubuntu 16/18, see
 [here](https://www.osetc.com/en/how-to-install-boost-on-ubuntu-16-04-18-04-linux.html). 
 Uninstallation of previous boost-dev packages might be necessary.   

### Usage 
Simply open the webpage in the browser :globe_with_meridians:

## Contributing
Pull requests :octocat: are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Team
| | |
| :---: | :---: |
| Volodymyr Chernetskyi | Hermann Yavorskyi |
| <img src="https://upload.wikimedia.org/wikipedia/ru/0/01/2003tmntscreenshot.jpg"> | <img src="https://upload.wikimedia.org/wikipedia/ru/a/a7/Red_Donatello.jpg"> |
| [chernetskyi](https://github.com/chernetskyi) | [wardady](https://github.com/wardady) |
| Andrii Koval | Petro Franchuk |
| <img src="https://upload.wikimedia.org/wikipedia/ru/2/23/Red_Raphael.jpg"> | <img src="https://upload.wikimedia.org/wikipedia/ru/f/f9/Red_Michelangelo.jpg"> |
| [andrwkoval](https://github.com/andrwkoval) | [franchukpetro](https://github.com/franchukpetro) |

See also the list of [contributors](https://github.com/chernetskyi/http-server/contributors) who participated in this project.

## License and Copyright
[![License](http://img.shields.io/:license-mit-blue.svg?style=flat-square)](http://badges.mit-license.org) 

This project is licensed under the [MIT License](https://choosealicense.com/licenses/mit/).

© 2019 Volodymyr Chernetskyi, Hermann Yavorskyi, Andrii Koval, Petro Franchuk
