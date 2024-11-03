if [ ! -d ~/.config ]
then mkdir ~/.config && cd ~/.config && git clone https://github.com/oscarpavon/nvim
else
cd ~/.config && git clone https://github.com/oscarpavon/nvim && cd ..
fi
nvim -c "PlugUpdate"
nvim -c "CocInstall coc-clangd"
