"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => General
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Map leader
let mapleader=","
" Automatic reloading
autocmd! bufwritepost .vimrc source %
set autoread

" Copy & Paste
set pastetoggle=<F2>
set clipboard=unnamedplus
set mouse=a "(alt + click)

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Config
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set so=7
set cmdheight=2
set hid
set backspace=eol,start,indent
set whichwrap+=<,>,h,l
set ignorecase
set hlsearch
set incsearch
set lazyredraw
set magic
set showmatch
set mat=2
set splitbelow " new splits are down
set splitright " new vsplits are to the right
set history=700
"
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Text
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set list
set listchars=tab:>-,trail:- " show tabs and trailing
set smartcase
set smartindent
set smarttab
set expandtab
set showcmd
set number
set linebreak
set textwidth=80
set cindent
set shiftwidth=4
set softtabstop=4
set autoread
set tabstop=4
set columns=80
set ai "Auto indent
set si "Smart indent
set wrap "Wrap lines
set colorcolumn=80

filetype plugin indent on

inoremap {      {}<Left>
inoremap {<CR>  {<CR>}<Esc>O
inoremap {{     {
inoremap {}     {}

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Colors and Fonts
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
filetype indent on
filetype plugin indent on
filetype on
filetype plugin on
syntax on
set grepprg=grep\ -nH\ $*

syntax on
set t_Co=256
colorscheme mustang
set encoding=utf8

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Folding
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set foldenable " Turn on folding
set foldmethod=marker
autocmd FileType c setlocal foldmethod=syntax
set foldlevel=1
set foldlevel=100 " Don't autofold anything (but I can still fold manually)
set foldnestmax=1 " I only like to fold outer functions
set foldopen=block,hor,mark,percent,quickfix,tag " what movements open folds

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Menu
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set wildmenu
set wildmode=list:longest,full
set wildignore=*.o,*~,*.pyc
set wildignore+=*.pdf,*.pyo,*.pyc,*.zip,*.so,*.swp,*.dll,*.o,*.DS_Store,*.obj,*.bak,*.exe,*.pyc,*.jpg,*.gif,*.png,*.a
set wildignore+=.git\*,.hg\*,.svn\*.o,*~,*pyc

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Backup
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
set nobackup
set nowb
set noswapfile

"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" => Mapping
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Remove arrow keys
noremap <Up> <NOP>
noremap <Down> <NOP>
noremap <Left> <NOP>
noremap <Right> <NOP>

" move by visual line
nnoremap j gj
nnoremap k gk
xnoremap j gj
xnoremap k gk

nnoremap J 5j
nnoremap K 5k
xnoremap J 5j
xnoremap K 5k

" copy/paste replaced
map <Leader>f c<C-r>0<ESC>
vmap <Leader>y "+y
vmap <Leader>d "+d
vmap <Leader>P "+p

" use shell with ctrl-z
map <C-Z> :shell<CR>

" Open/Close fold
nnoremap <space> za

" Remove highlight after search
noremap <Leader>D :nohl<CR>

"Quit & Save
noremap <Leader>w :w!<CR>
noremap <Leader>q :wq<CR>
noremap <Leader>Q :q!<CR>
nnoremap <leader>s :mksession<CR>

" Building
map <Leader>c :make<Return> :copen<Return>
map <Leader>d :make debug<Return> :copen<Return>
map <Leader>p :w !python<Return> :copen<Return>
map <Leader>cn :cn <CR>
map <Leader>cq :ccl <CR>

" Tabs
map <Leader>n <esc>:tabprevious<CR>
map <Leader>m <esc>:tabnext<CR>
map <Leader>b <esc>:tabnew<CR>

" Shifting
vnoremap < <gv
vnoremap > >gv

" Remapping Esc
inoremap jj <Esc>

"Whitespaces
nnoremap <Leader>r :let _s=@/<Bar>:%s/\s\+$//e<Bar>:let @/=_s<Bar>:nohl<CR>

""""""""""""""""""""""""""""""
" => Status line
""""""""""""""""""""""""""""""
set laststatus=2
set statusline+=%F\ [%lL\:%cC]

""""""""""""""""""""""""""""""
" => Plugin
""""""""""""""""""""""""""""""
execute pathogen#infect()
let g:ctrlp_max_height = 30

" CtrlP
let g:ctrlp_map = '<c-p>'
let g:ctrlp_cmd = 'CtrlP'
let g:ctrlp_follow_symlinks = 1
let g:ctrlp_match_window_bottom = 1
let g:ctrlp_match_window_reversed = 1
let g:ctrlp_max_depth = 100
let g:ctrlp_max_files = 100000
let g:ctrlp_max_height = 30
let g:ctrlp_working_path_mode = 'ra'
let g:ctrlp_use_caching = 1
let g:ctrlp_show_hidden = 1
let g:ctrlp_switch_buffer = 0 "new tab

" go
let g:go_disable_autoinstall = 1

"Rainbow
nnoremap <Leader>t :RainbowParenthesesToggle<CR>

" Syntastic
let g:syntastic_check_on_open=1
let g:syntastic_enable_signs=1
let g:syntastic_c_check_header = 1
let g:syntastic_c_auto_refresh_includes = 1
let g:syntastic_c_include_dirs = ['includes']
let g:syntastic_c_checkers=['make','gcc']

" Multiple cursor
let g:multi_cursor_next_key='<C-n>'
let g:multi_cursor_prev_key='<C-m>'
let g:multi_cursor_skip_key='<C-x>'
let g:multi_cursor_quit_key='<Esc>'

" Switch
nnoremap - :Switch<CR>

