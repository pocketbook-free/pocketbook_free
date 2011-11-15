Place files to ~/projects

In order to start working with SDK you shuld do the following:
1) Move the SDK folder to ~/projects
  mkdir ~/projects && cp -r ./sdkrelease_1_1a ~/projects/

2) Install packets:
  sudo apt-get install build-essential cmake autoconf qtcreator libgtk2.0-dev libbz2-dev libcurl4-openssl-dev libgif-dev

3) Enter the foolowing commands:
  sudo -s
  cd /usr/include && ln -sf freetype2/freetype freetype
