var exec = require('child_process').exec;
var cmd  = 'emcc main.c -o index.html -s USE_GLFW=3';

exec(cmd, function(error, stdout, stderr) {
   // command output is in stdout
});
