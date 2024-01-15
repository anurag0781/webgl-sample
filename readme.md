To compile, use this:

emcc -g triangle.cpp -O0 -s USE_GLFW=3 -s FULL_ES2=1 -s USE_WEBGL2=1

This will also allow hitting the breakpoints, seeing the variables in watch.

To start the python server:

python -m http.server 8080

(Python 3 should be installed.)

Now in the browser, type:

http://localhost:8080/





