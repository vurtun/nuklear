#import <UIKit/UIKit.h>

#include <allegro5/allegro.h>

@interface KeyboardHandleriOS : UIView <UIKeyInput>
-(void)setCustomKeyboardEventSource:(ALLEGRO_EVENT_SOURCE*)ev_src;
-(void)show;
-(void)hide;
@end

