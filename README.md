# Crystalcat
A Open source MDE Source code editor, browser and much more. 

How do I install the source code into a working software ? 
sudo make deps clean all install 

how do I uninstall the program ? 
sudo make uninstall 

Other handy make commands: 
make debug 
make clean all 
make all 
make dev 


MDE CRYSTALCAT
A high-performance modular development environment for ANSI C.

Forward

Crystalcat was engineered as a high-performance workspace that transcends the limits of a standard C editor.It is a comprehensive Modular Development Environment (MDE) designed for modern web and systems architecture.By integrating private, decentralized search protocols and an open-source SQLite backbone, Crystalcat ensures your research remains unfiltered and your data remains your own offline.

Fully tested with Docker container and Virtual Machines (VMs) - My goal for this program was lightweight, standalone, and executable package of software that includeseverything needed to run an application: code, runtime, system tools, system libraries, and settings all in (Crostini) with minimulist library setup dependency design.


Notes:

The Chromebook Linux environment (penguin) is neutered, referring to the strict security limitations imposed by ChromeOS, which restrict direct hardware access, lock down the root file system, and isolate the container to protect the core operating system.Google intentionally built the Linux development environment (Crostini) this way for several security and stability reasons As in cheaping out on SSD storage size and short-term based software support cycles that brick the cheap device. Also they work for the Big Iron NSA and other NGO'S the source on cromebooks are closed loop systems and are very low trust based platforms.
Database Engine

Integrated SQLite: Crystalcat features a native sqlite3 backbone.
Ad-Block & Filtering: The database-driven filtering system ensures a clean research environment.
Core Systems Architecture

SQLite Backbone: Powers persistent memory and internal states.
DUMB PTY/TTY Terminal: A direct forked process for zero-latency shell execution.
WebKit Engine: Renders documentation with custom privacy headers.
GTK 3.0 UI: Native Linux toolkit implementation.
Language Support

ANSI C / C++ With a focus on GCC and open source.
HTML5 ideal for website testing.
XHTML
Python
ASP
You can write and run assembly language code on a Chromebook by enabling the Linux Development Environment and installing an assembler like NASM or GCC.
CSS
Main keyboard Shortcuts

See FAQ for full keyboard shortcuts list in a table.

Shift + Ctrl + ^ Focus browser mode (show/hide editor)
Ctrl + / Toggle Comment
Ctrl + D Duplicate Line
Alt + Up Move Line Up
Alt + Down Move Line Down
CTRL + N: New Session
CTRL + W: Quick Save
CTRL + O: Open File
CTRL + S: Save File
CTRL + L: Clear Terminal Console
CTRL + R: Refresh Code Outline
CTRL + 1: Fast toggle JavaScript [ON/OFF]
Ctrl + C: Copy
Ctrl + X: Cut
Ctrl + V: Paste
Ctrl + Z: Undo
Ctrl + A: Select All
Ctrl + Shift + S: Save As
Ctrl + Q: Quit
Ctrl + J: Insert Snippet
Ctrl + F: Find
Ctrl + G: Go to line
Integrated Features

Docs Browser: Access search engines and C documentation.
Memory Management: Designed with strict C89/C90 compliance.
Terminal Emulator: Direct shell access via forked PTY process.
Rabbit & Cat Companion

What are the rabbit and cat characters?
These are animated, terminal-style pets integrated into the application interface. The rabbit serves as the primary assistant, while the cat appears to be a secondary character that interacts through a 'cat-chase' logic.

How do these characters behave?
The Rabbit: Cycles through animation frames (idle, ear twitch, blinking) and displays different colors based on the assistant's mood (e.g., IDLE, TYPING, COMPILING, ERROR). It also occasionally displays 'idle thoughts' to the user.
The Cat: Has its own animation sequence and an 'active' state that triggers a cat-chase scenario. Its activity is determined by random probability within the code.
How are these characters rendered?
The characters are rendered using inline 1-bit XBM data (a format for monochrome bitmap images) that is expanded into 8-bit alpha data for rendering via Cairo, a 2D graphics library.

What does the rabbit's color indicate?
The rabbit's color changes according to the system's current state:
Green: Default/Idle mood.
Blue: Compiling.
Red: Error state.
Random: When the assistant is in the typing mood, the color is randomized.
Companion Feature: The crystal cat

In addition to your primary rabbit assistant glitch, you might notice an autonomous cat character appearing in the workspace.Here are some frequently asked questions regarding this feature:

How do I command the cat?
So in the text editor just type,
//cat joke shows a random joke
//cat clean removes old 'a.out' junk files on the fly
When will the cat appear?
The cat’s activity is determined by random probability within the system logic. When the internal application counter reaches its threshold, there is a 37% chance the cat will trigger an 'active' state and appear on your canvas.
What does the cat do?
The cat face is a visual animation element 1st and background auto macro 2nd. Once active, it loops through specific movement frames (Idle, Eyes blink) before automatically resetting and disappearing from the screen.But the cat is always active hunting the ribbit in the background for code errors etc.

FAQ: CRYSTALCAT MDE 
Modular Development Environment — Frequently Asked Questions & System Manual

GENERAL & ARCHITECTURE

Q: What is Crystalcat MDE?
Crystalcat MDE is a lightweight, Modular Development Environment built in C using GTK+ 3, WebKit2GTK, SQLite3, and GtkSourceView. It combines a code editor, an embedded web browser, an integrated terminal, and an animated digital assistant into a unified developer workspace.
Q: What operating systems or environments is this program designed for?
The code is written in C and targets Linux / Unix-like POSIX environments (notably designed and tested for ChromeOS Linux / Crostini environments). It includes fallback systems and clipboard HUD bridges to work seamlessly within strict browser sandboxes like Google Chrome/Chromium.
Q: What libraries and compiler flags are required to build Crystalcat MDE?
To compile the application, you need standard development headers for:
GTK+ 3 (gtk+-3.0) / GTK+ 4.0 
WebKit2GTK (webkit2gtk-4.0)
GtkSourceView (gtksourceview-3.0 or gtksourceview-4.0)
SQLite3 (sqlite3)
Standard math and POSIX C libraries (-lm)
COMPANION ASSISTANT & UI AESTHETICS

Q: What is the pixelated pet on the screen, and what does it do?
The assistant panel features a retro 1-bit bitmap character (a rabbit/cat rendered directly via Cairo masks from raw XBM bit arrays). The assistant serves multiple purposes:
Idle: Green (#39FF14), displaying random developer thoughts or reminders.
Typing: Rapid multi-color shifts while actively editing code in the workspace.
Compiling: Cyan/Blue cycling through frame animation states.
Error: Bright Red (#FF3333) static or twitching frame.
Cat-Chase Animation: Periodic random cat animations that chase or interact across the main canvas area.
WEB BROWSER CONTENT READER FEATURES

Q: What is Article & Image Isolation, and how do I use Reader mode?
A: Article & Image Isolation lets you control which parts of a web page can load or display, helping create a cleaner reading experience and reduce unwanted page content.
Reader presets:

Strict Reader: Applies stricter content restrictions for a simplified reading experience.
Balanced Reader: Provides a balance between readability and website functionality.
Images + Text: Allows article text and images while restricting other content.
Normal Browsing: Restores normal browsing behavior when you want the full website experience.
Custom settings: The Article & Image Isolation dialog provides controls for JavaScript, page layout elements, fonts, media, raw resources, and popups. Adjust these options to customize what the page is allowed to display or load.

Applying your settings: Select your desired options and use Apply to apply them. Use Reset Controls to reset the dialog's controls, or Restore Normal Browsing to return to normal browsing.

Filter source: The source view displays the generated WebKit content-filter JSON associated with the current isolation settings. This allows you to inspect the rules being used.

Note: Strict content restrictions may prevent some websites from working correctly. If a page loses functionality, try Balanced Reader or Restore Normal Browsing.

CODE EDITOR & DATABASE FEATURES

Q: How does the Recent Files history work?
Crystalcat MDE manages recent files using two parallel systems:
In-Memory Ring Buffer: Stores up to 10 recent items dynamically rebuilt inside the application menus (RecentFilesWrapper).
SQLite Database Persistence: Tracks recent files inside ide_context.db in a table named recent_files. Selecting a file from the sidebar's "Recent" tab reloads it directly into the editor view.
Q: What safety prompt triggers when switching files?
If you activate a file from the "Recent" tree while your current buffer contains unsaved modifications, a GTK modal prompt appears asking whether to Save, Discard, or Cancel before opening the target file.
Q: Does the editor support search and line navigation?
Yes:
Find Dialog: Supports forward/backward text matching with optional Case Sensitive and Match Whole Word flags.
Go to Line: Allows rapid jumping to specific line numbers by converting human 1-indexed line numbers to zero-indexed GtkTextIters.
WEB BROWSER, PRIVACY & SECURITY

Q: Does the embedded browser support Ad Blocking?
Yes. Crystalcat MDE features a custom SQLite-backed ad filter system:
Filters are stored in the SQLite table ad_filters.
Patterns (e.g., .*doubleclick.net.*, .*google-analytics.com.*, tracker pixels, etc.) are compiled on the fly into JSON rules and passed to WebKit’s WebKitUserContentFilterStore.
You can manage or append custom URL block patterns using the Manage Filters menu dialog.
Q: What privacy and security features are included?
AI content stripping Just in time Allows instant disabling of AI html injection and agents.
JavaScript Toggle: Allows instant enabling/disabling of JavaScript execution across the browser.
Hardened Mode: Disables JavaScript, inline JS markup, and site-specific quirks for high-privacy browsing.
Do-Not-Track (DNT): Sends DNT signal headers via WebKit settings when supported.
Cookie Manager: Opens a dedicated "Cookie Monster" window to inspect active session cookies or flush all stored domain cookies at once.
Q: What is the "Security Bridge / Persistent HUD"?
When opening specific system-level or internal Chrome URLs (chrome:// or webkit://), ChromeOS sandboxes often block standard launch attempts. Crystalcat MDE intercepts these requests, automatically copies the URI to your system clipboard, and presents a borderless floating dark-mode HUD window. Once confirmed, it launches external focus so you can paste (CTRL+V) the URL directly into your main address bar.
SEARCH ENGINE & HTML UTILITIES

Q: What search tools and HTML utilities are included?
Advanced Search Dialog: Allows structured search queries combining keywords, target domains (site:), locations, and date ranges directly launching external Google queries.
HTML Boilerplate & Insertions: Includes quick workflows for entity escaping/unescaping, template insertion, image tags, links, tables, comments, and Lorem Ipsum placeholder text.
MULTIMEDIA & EMBEDDED TERMINAL

Q: How does the built-in Audio Stream Player work?
When opening an audio stream URL, Crystalcat MDE launches a dedicated compact window rendering an HTML5/CSS audio player powered by WebKit. It includes play/pause/stop controls, volume sliders, and real-time network and buffer health indicators (buffered, networkState, readyState). You also have the option of embedding the somafm streaming website within the app crystalcat.
Q: Does the MDE include an integrated terminal or compilation tool?
Yes:
Embedded Terminal: Spawns sub-shell processes using POSIX pseudo-terminals (master_fd via openpty/fcntl) hooked into a GTK console buffer.
GCC Compilation Engine: Includes internal routines (logic_run_gcc_advanced) to directly trigger system GCC compilation commands on open source files.
Q: Why is the browser so paranoid?
Yes paranoid I am! :
If it feels like the browser is acting paranoid — blocking scripts, popping up alerts, or breaking pages— it’s usually down to a few specific reasons built into its design:


Hardened Defaults
By default, the browser is configured to assume that untrusted code could and will be malicious. Features like Hardened Privacy Mode and strict HTTP header enforcement lock down common attack vectors (like cross-site scripting or fingerprinting), but they can sometimes cause aggressive site-blocking this is A good thing. AI agents are always hacking code in real time!
Aggressive Cookie & Tracker Blocking
With Intelligent Tracking Protection (ITP) and active database-level web filtering, the browser constantly scans incoming network requests. If a site tries to load third-party tracking scripts, persistent cookies, or telemetry pixels, the browser immediately cuts off the connection.
JavaScript Restriction as default
If JavaScript is toggled off or heavily restricted via the menu, many modern websites (which rely on JS to render images, menus, and dynamic content) will instantly freeze or break. To the user, it feels like the browser is panicking, but it's actually just refusing to run unverified AI code. The Internet is a boneyard of broken dead code or DIT.
Local Database Rule Enforcement
Because the browser checks incoming URLs against local SQLite filter lists, it acts as a local firewall. If a domain matches a pattern flagged for ad-tracking or invasive data collection, access is immediately blocked without asking for permission first by default.
🛡️ PRIVACY FEATURES FAQ
Learn more about the built-in tracking protection and security controls in our browser.

1. WHAT NEW PRIVACY PROTECTIONS HAVE BEEN ADDED?

Our latest release introduces several built-in privacy tools designed to give you complete control over your data and brick AI agents:

Hardened Privacy Mode: A one-click toggle that enforces strict security rules across all sites.
Smart Cookie Management: Enhanced tracking protection to manage, block, or modify how third-party cookies track you.
Integrated Do Not Track (DNT) & ITP: Native support for Intelligent Tracking Protection and standard DNT signals.
Custom Web Filters: Built-in ad and domain filtering powered by a dedicated database.
JavaScript Control: Global or site-by-site toggles to disable high-risk scripts when needed.
2. WHAT IS "HARDENED PRIVACY MODE"?

Hardened Privacy Mode is an all-in-one security setting. When enabled, it automatically applies maximum restriction settings—blocking aggressive fingerprinting techniques, restricting third-party tracking scripts, and enforcing strict HTTP headers to ensure your browsing identity stays private.

3. HOW DOES THE COOKIE MANAGER WORK?

Our dynamic Cookie Manager puts you in charge of local data:

Inspect: View stored tracking cookies per session or domain.
Modify & Alter: Selectively alter cookie attributes or expire invasive tracking tokens manually.
Auto-Clear: Automatically purge non-essential tracking cookies when you close a session.
4. HOW DO I ENABLE OR MANAGE WEB FILTERS?

You can access web filtering directly from the menu bar:

Navigate to Tools > Manage Web Filters...
Add custom URL patterns, domain targets, or blocklists.
Choose whether to block requests outright or alter headers to mask your digital footprint.
Your filter list is stored locally on your device in a lightweight SQLite database, meaning your rules are never uploaded to our servers.

5. WILL TOGGLING OFF JAVASCRIPT BREAK WEBSITES?

Disabling JavaScript (via Browser > Enable JavaScript) significantly boosts speed and stops tracking scripts, but it may cause dynamic web applications (like video platforms or online editors) to render incorrectly. We recommend toggling JavaScript off for simple reading and toggling it back on for complex web apps.

6. HOW DO I ENABLE THESE FEATURES?

Most privacy controls can be easily toggled from the top menu:

To manage tracking signals: Open the Browser menu and check Do Not Track / ITP or Hardened Privacy Mode.
To manage cookies: Select Browser > Cookie Manager...
To perform private searches: Use the built-in DuckDuckGo search shortcut directly from the search bar.
7. WHAT IS "AGENT SPOOFING"?

Agent Spoofing (or User-Agent spoofing) alters the information your browser sends to websites about your device and operating system. By disguising your browser identity as a generic or different device, it prevents websites and tracking networks from building a unique fingerprint based on your specific browser setup.

8. HOW DOES GEOLOCATION RANDOMIZATION WORK?

Geolocation Randomization dynamically spoofs your browser's physical location API. Instead of revealing your true coordinates or relying on fixed locations, it injects random latitude and longitude points into the web environment for each session, preventing websites from using location data to track or pinpoint your geographic position. This is designed to be used in conjunction with a proxy and VPN for comprehensive location privacy. It even logs a funny message in the developer console to let you know your real location remains sudo-hidden within the layers of your browser.

EDITOR KEYBOARD SHORTCUTS

Q: What are the keyboard shortcuts for the editor?
Ctrl + / Toggle Comment
Ctrl + D Duplicate Line
Alt + Up Move Line Up
Alt + Down Move Line Down
CTRL + N: New Session
CTRL + W: Quick Save
CTRL + O: Open File
CTRL + S: Save File
CTRL + L: Clear Terminal
CTRL + R: Refresh Code Outline
CTRL + 1: Toggle JavaScript-[ON/OFF]
Ctrl + C: Copy
Ctrl + X: Cut
Ctrl + V: Paste
Ctrl + Z: Undo
Ctrl + A: Select All
Ctrl + Shift + S: Save As
Ctrl + Q: Quit
Ctrl + J: Insert Snippet
Ctrl + F: Find
Ctrl + G: Go to line
Shift + Ctrl + 6: Focus Browser mode.

• Crystalcat MDE Internal Documentation • Built with GTK+ & WebKit2GTK •
Open Source for Liinux and Cromebook computing devices .o*(2026)*o.
