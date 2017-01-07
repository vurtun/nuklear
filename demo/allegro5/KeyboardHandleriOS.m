#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif
#import "KeyboardHandleriOS.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_iphone_objc.h>
@interface KeyboardHandleriOS()
{
    ALLEGRO_EVENT_SOURCE *event_source;
    ALLEGRO_DISPLAY *current_display;
}
@end
@implementation KeyboardHandleriOS
- (id)init {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardDidHide:) name:UIKeyboardDidHideNotification object:nil];
    self = [self initWithFrame:CGRectMake(-100, -100, 0, 0)];
    event_source = NULL;
    current_display = al_get_current_display();
    UIView* v = al_iphone_get_view(current_display);
    [v addSubview:self];
    return self;
}

- (void)setCustomKeyboardEventSource:(ALLEGRO_EVENT_SOURCE *)ev_src {
    event_source = ev_src;
}

- (UIKeyboardType) keyboardType
{
    return UIKeyboardTypeASCIICapable;
}

- (UITextAutocorrectionType) autocorrectionType
{
    return UITextAutocorrectionTypeNo;
}

-(BOOL)canBecomeFirstResponder {
    return YES;
}

- (void)deleteBackward {
    
    if (!event_source) {
        NSLog(@"deleteBackward(): No event source found, not sending events");
        return;
    }
    
    ALLEGRO_EVENT *event_down = (ALLEGRO_EVENT*)calloc(1, sizeof(ALLEGRO_EVENT));
    ALLEGRO_EVENT *event_up = (ALLEGRO_EVENT*)calloc(1, sizeof(ALLEGRO_EVENT));
    
    event_down->type = ALLEGRO_EVENT_KEY_DOWN;
    event_down->keyboard.display = current_display;
    event_down->keyboard.keycode = ALLEGRO_KEY_BACKSPACE;
    event_up->type = ALLEGRO_EVENT_KEY_UP;
    event_up->keyboard.display = current_display;
    event_up->keyboard.keycode = ALLEGRO_KEY_BACKSPACE;
    al_emit_user_event(event_source, event_down, NULL);
    al_emit_user_event(event_source, event_up, NULL);
    
    free(event_down);
    free(event_up);
}

- (BOOL)hasText {
    return YES;
}

- (void)insertText:(NSString *)text
{
    if (!event_source) {
        NSLog(@"insertText(): No event source found, not sending events");
        return;
    }
    
    ALLEGRO_EVENT *event_down = (ALLEGRO_EVENT*)calloc(1, sizeof(ALLEGRO_EVENT));
    ALLEGRO_EVENT *event_up = (ALLEGRO_EVENT*)calloc(1, sizeof(ALLEGRO_EVENT));
    
    if([text isEqualToString:@"\n"])
    {
        event_down->type = ALLEGRO_EVENT_KEY_DOWN;
        event_down->keyboard.display = current_display;
        event_down->keyboard.keycode = ALLEGRO_KEY_ENTER;
        event_up->type = ALLEGRO_EVENT_KEY_UP;
        event_up->keyboard.display = current_display;
        event_up->keyboard.keycode = ALLEGRO_KEY_ENTER;
        al_emit_user_event(event_source, event_down, NULL);
        al_emit_user_event(event_source, event_up, NULL);
        [self hide];
        //m_kb->setDonePressed();
    }
    else {
        event_down->type = ALLEGRO_EVENT_KEY_CHAR;
        event_down->keyboard.display = current_display;
        event_down->keyboard.unichar = [text characterAtIndex:0];
        // doesn't matter what keycode is, nuklear backend ignores it as long as it
        // isn't a special key
        event_down->keyboard.keycode = ALLEGRO_KEY_A;
        al_emit_user_event(event_source, event_down, NULL);
    }
    free(event_down);
    free(event_up);
}

-(void)show {
    NSLog(@"Should be showing!");
 [self performSelectorOnMainThread:@selector(becomeFirstResponder) withObject:nil waitUntilDone:YES];
}
-(void)hide {
    NSLog(@"Should be hiding!");
    [self performSelectorOnMainThread:@selector(resignFirstResponder) withObject:nil waitUntilDone:YES];
}
- (void)keyboardDidHide:(NSNotification *)notification {
    NSLog(@"keyboardDidHide called");
}

-(void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardDidHideNotification object:nil];
}
@end
