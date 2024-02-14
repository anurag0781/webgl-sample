To compile, use this:

emcc -g triangle.cpp -O0 -s USE_GLFW=3 -s FULL_ES2=1 -s USE_WEBGL2=1 -s OFFSCREENCANVAS_SUPPORT=1 -s USE_WEBGL2=1 -s FULL_ES2=1 -pthread -s PTHREAD_POOL_SIZE=1
This will also allow hitting the breakpoints, seeing the variables in watch.

On a terminal:

npm install -g serve

serve -s . --listen 8080 --ssl-cert ./localhost.adobe.com.pem --ssl-key ./localhost.adobe.com-key.pem


Then, on the browser:

http://localhost:8080/





