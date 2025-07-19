**XONIX - C++ SFML Game Project**

A 2D Xonix-style arcade game built using C++ and the SFML 3.0.0 library.

**Gameplay Link**

https://www.linkedin.com/posts/muhammad-ahmed-cheema-75b454327_cpp-sfml-gamedevelopment-activity-7352313272722751488-Ys6Z?utm_source=social_share_send&utm_medium=member_desktop_web&rcm=ACoAAFJ4VDcBh7Y3WTlCxF2vgDRkfipUUQ3RJ14
________________________________________
**Project Description**

Xonix is a modern, interactive recreation of the classic Xonix game developed as a final project for the Programming Fundamentals course. Developed in C++ using SFML, the game features both single-player and two-player modes, multiple difficulty levels, power-ups, geometric enemy movement, scoreboards, and rich audio-visual feedback.
The objective of the game is to claim territory by capturing tiles while avoiding enemies. The project includes extensive game mechanics, menus, file handling, timers, and custom logic to deliver an engaging player experience.
________________________________________
**Features**

•	Modular code with separate logic blocks for game loop, input, collision, scoring, and enemy movement

•	Single and two-player modes with distinct control schemes

•	Menu systems: Main Menu, Level Selection, Mode Selection, Score Display

•	Scoring system with dynamic multipliers and file-based score storage

•	Power-ups that temporarily freeze enemies and opponents

•	Difficulty levels: Easy, Medium, Hard, Continuous

•	Geometric enemy movements and increasing challenge over time

•	Scoreboard system displaying top scores and playtime

•	Audio effects and background music integration

________________________________________
**Project Structure and Task Distribution**

**Team Members:**

•	Muhammad Ahmed Cheema (https://www.linkedin.com/in/muhammad-ahmed-cheema-75b454327/)

•	Warisha Ishtiaq (https://www.linkedin.com/in/warisha-ishtiaq-93b111312/)

**Task Allocation:**

**Member	Tasks**

Ahmed Cheema : Menus,  2-Player Mode, File-based Scoring, Scoreboard

Warisha :	Power-Up and Scoring Logic,Enemy Level Setup, Enemy Movement, Report
________________________________________
**Core Systems and Logic**

**Menu System**

•	Implemented via Menu(), levelMenu(), playerModeMenu()

•	Options for difficulty, game mode, scorecard, and exit

•	End-game screen shows score, tile and move counts

**Difficulty & Enemies**

•	Easy: 2 Enemies

•	Medium: 4 Enemies

•	Hard: 6 Enemies

•	Continuous: Starts from 2 then Enemies increase every 20 seconds (timer-based logic)
Movement and Scoring

•	tileCount1 tileCount2 and moveCount1 moveCount2 track player progress

•	Move counted only on Successful tile capture

**Enemy Mechanism**

•	Speed increases every 20 seconds

•	Geometric motion (Zig-Zag and Circular) after 30 seconds using timer logic

•	move() function handles pathing and pattern changes

**Power-Ups**

•	Triggered at scores: First at 50, then from 70 to every 30th after i.e. 100, 130, 160...

•	Freeze enemies/opponents for 3 seconds

•	Visual cue: background Changes.

•	Logic managed by PowerUpCount, PowerUpActive

**Scoreboard**

•	Stores top 5 scores with time and player names

•	Displays results post-game

•	Player names with ! e.g !Player1 indicate death during gameplay
________________________________________
**Two Player Mode**

•	Player 1: Arrow Keys

•	Player 2: WASD

•	Shared game timer

**Collision Rules:**

•	Hitting trails causes instant death

•	Simultaneous head-on: moving player loses

•	Enemy collision with trail: game over

•	Highest scorer wins if both die
________________________________________
**Audio-Visual Enhancements (Bonus)**

•	Magenta background during power-ups, Opposite Player and Enemies Paused for 3 seconds.

•	.wav sound effects for tile capture, game over, and power-up activation

•	Managed with sf::SoundBuffer and sf::Sound
________________________________________
**Prerequisites**

•	C++ Compiler (g++, MSVC, etc.)

•	SFML 3.0.0
________________________________________
**How to Build & Run**

Windows (g++)

g++ main.cpp -o XONIX -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

XONIX.exe

**Linux (Debian-based)**

sudo apt install libsfml-dev g++

g++ main.cpp -o XONIX -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
./XONIX

Ensure all asset files (images, audio, fonts, etc.) are in the same directory as the executable.
________________________________________
**Project Files**

•	main.cpp - Main game logic

•	score.txt - Scoreboard file

•	.wav, .jpeg, .png - Game assets

•	.ttf - Custom font file
________________________________________
**Libraries and Limitations**

•	Used: C++ standard headers, SFML (graphics, audio, window, system)

•	Avoided: STL containers, other restricted headers
________________________________________
**Conclusion**

The XONIX project fulfills its intended goal as a feature-rich, interactive arcade game. It combines game logic, real-time graphics, audio feedback, and file handling, reinforcing our understanding of C++ fundamentals and event-driven programming.
________________________________________
**Author**


•	Muhammad Ahmed Cheema

This project was developed as part of the Programming Fundamentals final project.

