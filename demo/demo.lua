gui = require(gui);

demo = {};

function init_demo(stack, font)
    demo.font = font;
    demo.canvas = gui.buffer.canvas();
    demo.config = gui.panel.config();
    demo.layout = gui.panel.layout();
    demo.tab = gui.panel.layout();

    demo.memory = gui.buffer.memory(8 * 1024);
    demo.buffer = gui.buffer.new(demo.memory);
    demo.panel = gui.hook.new(50, 50, 200, 400, demo.config, font);
    gui.stack.push(stack, demo.panel);
end

function update_demo(stack, input, width, height)
    gui.buffer.begin(demo.canvas, demo.buffer, width, height);
    gui.panel.hook.begin(demo.layout, demo.panel, stack, "Demo", demo.canvas, input);
    if gui.panel.button(demo.layout, "Button") then print("button pressed!") end
    gui.panel.hook.end(demo.layout, demo.panel);
    gui.buffer.end(demo.panel:output(), demo.buffer, demo.canvas);
end

