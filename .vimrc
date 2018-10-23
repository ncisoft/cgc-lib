"let g:EasyGrepFilesToExclude = "*.bak,*~,cscope.*,*.a,*.o,*.pyc,*.bak,*.c, build/*,tags"
let g:EasyGrepFilesToExclude=".git,build,tags,*.bak,*.o,*.s"
let g:EasyGrepRoot = "search:.git"
set wildignore=*.swp,.gitignore
"https://gist.github.com/seyDoggy/613f2648cebc6c7b456f
unlet g:ctrlp_custom_ignore
unlet g:ctrlp_user_command
" set your own custom ignore settings
set wildignore+=*/tmp/*,*.so,*.swp,*.zip,*.o,tags
let g:ctrlp_custom_ignore = {
    \ 'dir':  '\.git$\|\.hg$\|\.svn$\|bower_components$\|build$\|node_modules$\|project_files$\|test$|tags$',
    \ 'file': '\.out$\|\.so$\|\.dll$\|\.out$|\.o$|tags$' }
" clean ctrlp cache
" CtrlPClearCache
" rm -rf ~/.cache/ctrlp/

"http://harttle.com/2015/11/17/vim-buffer.html
set wildmenu wildmode=full 
set wildchar=<Tab> wildcharm=<C-Z>

" http://timothyqiu.com/archives/using-clang-complete-for-c-cplusplus-in-vim/
let g:clang_c_options = ' -std=c99 '  
let g:clang_cpp_options = ' -std=c++11 -stdlib=libc++  ' 
        let g:clang_compilation_database = './build'
        let g:clang_cpp_completeopt = 'longest,menuone,preview'
"        let g:clang_include_sysheaders = 1

" https://github.com/vim-syntastic/syntastic/wiki/C--:---gcc
"http://stackoverflow.com/questions/18158772/how-to-add-c11-support-to-syntastic-vim-plugin
"let g:syntastic_cpp_compiler_options = " -std=c++11 -stdlib=libc++ -I/usr/include/lua5.1/
"-I/home/leeyg/develop/cpp-gc/contrib/colors "
let g:syntastic_cpp_compiler="clang++"
let g:syntastic_cpp_compiler_options =" -std=c++11 "
let g:syntastic_cpp_include_dirs=[ g:git_dir . '/include/',  '/usr/include/c++/4.9', '/usr/include/i386-linux-gnu/c++/4.9',  g:git_dir . '/contrib/call_in_stack/']

let g:syntastic_c_compiler="clang"
let g:syntastic_c_compiler_options =" -std=c99 "
let g:syntastic_c_include_dirs=[ g:git_dir . '/include/', '/usr/include/lua5.1', g:git_dir . '/contrib/lua51-ext/',  g:git_dir . '/contrib/call_in_stack/']

" https://superuser.com/questions/77800/vims-autocomplete-how-to-prevent-vim-to-read-some-include-files
set complete-=i

let g:clang_complete_copen=1
let g:clang_periodic_quickfix=1
let g:clang_snippets=1
let g:clang_close_preview=1
let g:clang_use_library=1

" dont highlight inactive window
let g:diminactive_use_colorcolumn = 0
let g:ctrlp_clear_cache_on_exit = 1
let g:tagbar_ctags_bin = "ctags"

set directory=$HOME/tmp/dirty
