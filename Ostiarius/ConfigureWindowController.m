//
//  Utilities.m
//  Ostiarius
//
//  Created by Patrick Wardle on 1/2/16.
//  Copyright (c) 2016 Objective-See. All rights reserved.
//

#import "Consts.h"
#import "Logging.h"
#import "Configure.h"
#import "Utilities.h"
#import "ConfigureWindowController.h"

@implementation ConfigureWindowController

@synthesize action;
@synthesize statusMsg;
@synthesize windowTitle;
@synthesize moreInfoButton;

//automatically called when nib is loaded
// ->just center window
-(void)awakeFromNib
{
    //center
    [self.window center];
    
    return;
}

//configure window/buttons
// ->also brings window to front
-(void)configure:(NSString*)title action:(NSUInteger)requestedAction
{
    //save window title
    self.windowTitle = title;
    
    //save action
    self.action = requestedAction;
    
    //set window title
    [self window].title = self.windowTitle;
    
    //dbg msg
    logMsg(LOG_DEBUG, @"configuring install/uninstall window");
    
    //init button title
    // ->based on action
    switch (self.action)
    {
        //install
        case ACTION_INSTALL_FLAG:
            
            //set
            self.actionButton.title = ACTION_INSTALL;
            
            //init status msg
            [self.statusMsg setStringValue:@"blocks unsigned internet binaries"];

            break;
            
        //uninstall
        case ACTION_UNINSTALL_FLAG:
            
            //set
            self.actionButton.title = ACTION_UNINSTALL;
            
            //init status msg
            [self.statusMsg setStringValue:@"disable & remove protection"];

            break;
            
        default:
            break;
    }
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"%@/%@", self.actionButton.title, self.window.title]);
    
    //set delegate
    [self.window setDelegate:self];

    return;
}

//display (show) window
// ->center, make front, set bg to white, etc
-(void)display
{
    //center window
    [[self window] center];
    
    //show (now configured) windows
    [self showWindow:self];
    
    //make it key window
    [self.window makeKeyAndOrderFront:self];
    
    //make window front
    [NSApp activateIgnoringOtherApps:YES];
    
    //make white
    [self.window setBackgroundColor: NSColor.whiteColor];

    return;
}

//button handler for 'cancel'
// ->just close window, which will trigger app exit
-(IBAction)cancel:(id)sender
{
    //dbg msg
    logMsg(LOG_DEBUG, @"handling 'cancel' button click, exiting process");
    
    //close
    [self.window close];
    
    return;
}

//button handler for action ('install' || 'uninstall')
-(IBAction)handleActionClick:(id)sender
{
    //button title
    NSString* button = nil;
    
    //extact button title
    button = ((NSButton*)sender).title;
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"handling action click: %@", button]);
    
    //handle non-'close' clicks
    if(YES != [button isEqualToString:ACTION_CLOSE])
    {
        //disable 'x' button
        // ->don't want user killing app during install/upgrade
        [[self.window standardWindowButton:NSWindowCloseButton] setEnabled:NO];
        
        //clear status msg
        [self.statusMsg setStringValue:@""];
        
        //force redraw of status msg
        // ->sometime doesn't refresh (e.g. slow VM)
        [self.statusMsg setNeedsDisplay:YES];
        
        //dbg msg
        logMsg(LOG_DEBUG, [NSString stringWithFormat:@"%@'ing Ostiarius", button]);
        
        //invoke logic to install/uninstall
        // ->do in background so UI doesn't block
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
        ^{
            //install/uninstall
            [self lifeCycleEvent:self.action];
        });
    }
    
    //handle 'close'
    // ->just close window, which will trigger app exit
    else
    {
        //close
        [self.window close];
    }

//bail
bail:
    
    return;
}

//button handler that's automatically invoked when user clicks '?' button (on an error)
// ->load objective-see's documentation for error(s)
-(IBAction)handleInfoClick:(id)sender
{
    //url
    NSURL *helpURL = nil;
    
    //build help URL
    helpURL = [NSURL URLWithString:[NSString stringWithFormat:@"%@#errors", PRODUCT_URL]];
    
    //open URL
    // ->invokes user's default browser
    [[NSWorkspace sharedWorkspace] openURL:helpURL];
    
    return;
}

//perform install | uninstall via Control obj
// ->invoked on background thread so that UI doesn't block
-(void)lifeCycleEvent:(NSUInteger)event
{
    //status var
    BOOL status = NO;
    
    //configure object
    Configure* configureObj;
    
    //dbg msg
    logMsg(LOG_DEBUG, [NSString stringWithFormat:@"handling life cycle event, %lu", (unsigned long)event]);
    
    //alloc control object
    configureObj = [[Configure alloc] init];
    
    //begin event
    // ->updates ui on main thread
    dispatch_sync(dispatch_get_main_queue(),
    ^{
        //complete
        [self beginEvent];
    });
    
    //perform action (install | uninstall)
    // ->perform background actions
    if(YES == [configureObj configure:event])
    {
        //set flag
        status = YES;
    }
    
    //error occurred
    else
    {
        //err msg
        logMsg(LOG_ERR, @"ERROR: failed to perform life cycle event");
        
        //set flag
        status = NO;
    }
    
    //complet event
    // ->updates ui on main thread
    dispatch_async(dispatch_get_main_queue(),
    ^{
        //complete
        [self completeEvent:status];
    });
    
    return;
}

//begin event
// ->basically just update UI
-(void)beginEvent
{
    //status msg frame
    CGRect statusMsgFrame = {0};
    
    //grab exiting frame
    statusMsgFrame = self.statusMsg.frame;
    
    //avoid activity indicator
    // ->shift frame shift delta
    statusMsgFrame.origin.x += FRAME_SHIFT;
    
    //update frame to align
    self.statusMsg.frame = statusMsgFrame;
    
    //align text left
    [self.statusMsg setAlignment:NSLeftTextAlignment];
    
    //update status msg UI
    [self.statusMsg setStringValue:[NSString stringWithFormat:@"%@ing...", [self.actionButton.title lowercaseString]]];
    
    //disable action button
    self.actionButton.enabled = NO;
    
    //disable cancel button
    self.cancelButton.enabled = NO;
    
    //show spinner
    [self.activityIndicator setHidden:NO];
    
    //start spinner
    [self.activityIndicator startAnimation:nil];
    
    return;
}

//complete event
// ->update UI after background event has finished
-(void)completeEvent:(BOOL)success
{
    //status msg frame
    CGRect statusMsgFrame = {0};
    
    //result msg
    NSString* resultMsg = nil;
    
    //msg font
    NSColor* resultMsgColor = nil;
    
    //success
    if(YES == success)
    {
        //set result msg
        resultMsg = [NSString stringWithFormat:@"Ostiarius %@ed", self.actionButton.title];
        
        //set font to black
        resultMsgColor = [NSColor blackColor];
    }
    //failure
    else
    {
        //set result msg
        resultMsg = [[NSString stringWithFormat:@"error: %@ failed", self.actionButton.title] lowercaseString];
        
        //set font to red
        resultMsgColor = [NSColor redColor];
        
        //show 'get more info' button
        // ->don't have to worry about (re)hiding since the only option is to close the app
        [self.moreInfoButton setHidden:NO];
    }
    
    //stop/hide spinner
    [self.activityIndicator stopAnimation:nil];
    
    //align text center
    [self.statusMsg setAlignment:NSCenterTextAlignment];
    
    //grab exiting frame
    statusMsgFrame = self.statusMsg.frame;
    
    //shift back since activity indicator is gone
    statusMsgFrame.origin.x -= FRAME_SHIFT;
    
    //update frame to align
    self.statusMsg.frame = statusMsgFrame;
    
    //set font to bold
    [self.statusMsg setFont:[NSFont fontWithName:@"Menlo-Bold" size:13]];
    
    //set msg color
    [self.statusMsg setTextColor:resultMsgColor];
    
    //set status msg
    [self.statusMsg setStringValue:resultMsg];
    
    //set button title to 'close'
    self.actionButton.title = ACTION_CLOSE;
    
    //enable
    self.actionButton.enabled = YES;
    
    //ok to re-enable 'x' button
    [[self.window standardWindowButton:NSWindowCloseButton] setEnabled:YES];
    
    //make it active
    [self.window makeFirstResponder:self.actionButton];
    
    //(re)make window window key
    [self.window makeKeyAndOrderFront:self];
    
    //(re)make window front
    [NSApp activateIgnoringOtherApps:YES];
    
    return;
}

//automatically invoked when window is closing
// ->tell OS that we are done with window so it can (now) be freed
-(void)windowWillClose:(NSNotification *)notification
{
    //dbg msg
    logMsg(LOG_DEBUG, @"configure window: windowWillClose()");
    
    //exit
    [NSApp terminate:self];
    
    return;
}


@end
