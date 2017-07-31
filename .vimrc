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
let g:clang_c_options = ' -std=c99 -I/usr/include/lua5.1/ -I$PWD/include/ '  
let g:clang_cpp_options = ' -std=c++11 -stdlib=libc++  -I$PWD/include/ -I/usr/local/include/ -I/usr/include -I/usr/include/c++/4.9 -I/usr/include/i386-linux-gnu/c++/4.9 ' 
let g:clang_user_options = ' -std=c99  -I/usr/include/lua5.1/ -I$PWD/include/ ' 
        let g:clang_compilation_database = './build'
        let g:clang_cpp_completeopt = 'longest,menuone,preview'
"        let g:clang_include_sysheaders = 1

"http://stackoverflow.com/questions/18158772/how-to-add-c11-support-to-syntastic-vim-plugin
"let g:syntastic_cpp_compiler_options = " -std=c++11 -stdlib=libc++ -I/usr/include/lua5.1/
"-I/home/leeyg/develop/cpp-gc/contrib/colors "
let g:syntastic_cpp_compiler="clang++"
let g:syntastic_cpp_compiler_options = " -std=c++11 -stdlib=libc++ "
let g:syntastic_cpp_include_dirs=['-I$PWD/include/', '-I /usr/include',  '-I/usr/local/include/', '-I/usr/include/c++/4.9', '-I/usr/include/i386-linux-gnu/c++/4.9' ]

let g:syntastic_c_compiler="c"
let g:syntastic_c_compiler_options = " -std=c99  "
let g:syntastic_c_include_dirs=['-I$PWD/include/', ' -I/usr/include/lua5.1']

" https://superuser.com/questions/77800/vims-autocomplete-how-to-prevent-vim-to-read-some-include-files
set complete-=i

" dont highlight inactive window
let g:diminactive_use_colorcolumn = 0
let g:ctrlp_clear_cache_on_exit = 1
