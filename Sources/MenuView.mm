// ═══════════════════════════════════════
//  ModEngine Menu UI
//  iOS frosted glass style, triple-tap to open
//  Streamproof via ScreenProtectorKit-Spoof
// ═══════════════════════════════════════

#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include "Aimbot.h"
#include "ESP.h"
#include "Bypass.h"

using namespace ME;

// ── Extern configs (defined in ModEngine.mm) ──────────────
extern AimbotConfig  g_AimbotCfg;
extern ESPConfig     g_ESPCfg;
extern bool          g_StreamProof;

// ─────────────────────────────────────────────────────────
#pragma mark - Toggle Cell
// ─────────────────────────────────────────────────────────

@interface METoggleCell : UITableViewCell
@property (nonatomic, copy) NSString* label;
@property (nonatomic) BOOL  value;
@property (nonatomic, copy) void (^onToggle)(BOOL);
- (void)setup;
@end

@implementation METoggleCell {
    UILabel*  _label;
    UISwitch* _switch;
}
- (void)setup {
    self.backgroundColor = [UIColor clearColor];
    self.selectionStyle  = UITableViewCellSelectionStyleNone;

    _label = [UILabel new];
    _label.translatesAutoresizingMaskIntoConstraints = NO;
    _label.textColor = [UIColor whiteColor];
    _label.font      = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    _label.text      = self.label;
    [self.contentView addSubview:_label];

    _switch = [UISwitch new];
    _switch.translatesAutoresizingMaskIntoConstraints = NO;
    _switch.on = self.value;
    _switch.onTintColor = [UIColor colorWithRed:0.2 green:0.6 blue:1.0 alpha:1.0];
    [_switch addTarget:self action:@selector(didSwitch:) forControlEvents:UIControlEventValueChanged];
    [self.contentView addSubview:_switch];

    [NSLayoutConstraint activateConstraints:@[
        [_label.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
        [_label.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],

        [_switch.centerYAnchor constraintEqualToAnchor:self.contentView.centerYAnchor],
        [_switch.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
    ]];
}
- (void)didSwitch:(UISwitch*)sw {
    self.value = sw.on;
    if (self.onToggle) self.onToggle(sw.on);
}
@end

// ─────────────────────────────────────────────────────────
#pragma mark - Slider Cell
// ─────────────────────────────────────────────────────────

@interface MESliderCell : UITableViewCell
@property (nonatomic, copy) NSString* label;
@property (nonatomic) float value;
@property (nonatomic) float minVal;
@property (nonatomic) float maxVal;
@property (nonatomic, copy) void (^onChange)(float);
- (void)setup;
@end

@implementation MESliderCell {
    UILabel*  _label;
    UISlider* _slider;
    UILabel*  _valueLabel;
}
- (void)setup {
    self.backgroundColor = [UIColor clearColor];
    self.selectionStyle  = UITableViewCellSelectionStyleNone;

    _label = [UILabel new];
    _label.translatesAutoresizingMaskIntoConstraints = NO;
    _label.textColor = [UIColor whiteColor];
    _label.font      = [UIFont systemFontOfSize:13 weight:UIFontWeightRegular];
    _label.text      = self.label;
    [self.contentView addSubview:_label];

    _valueLabel = [UILabel new];
    _valueLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _valueLabel.textColor = [UIColor colorWithWhite:0.7 alpha:1];
    _valueLabel.font      = [UIFont monospacedDigitSystemFontOfSize:12 weight:UIFontWeightRegular];
    _valueLabel.text      = [NSString stringWithFormat:@"%.0f", self.value];
    [self.contentView addSubview:_valueLabel];

    _slider = [UISlider new];
    _slider.translatesAutoresizingMaskIntoConstraints = NO;
    _slider.minimumValue = self.minVal;
    _slider.maximumValue = self.maxVal;
    _slider.value        = self.value;
    _slider.minimumTrackTintColor = [UIColor colorWithRed:0.2 green:0.6 blue:1.0 alpha:1.0];
    [_slider addTarget:self action:@selector(didSlide:) forControlEvents:UIControlEventValueChanged];
    [self.contentView addSubview:_slider];

    [NSLayoutConstraint activateConstraints:@[
        [_label.topAnchor constraintEqualToAnchor:self.contentView.topAnchor constant:8],
        [_label.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],

        [_valueLabel.centerYAnchor constraintEqualToAnchor:_label.centerYAnchor],
        [_valueLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],

        [_slider.topAnchor constraintEqualToAnchor:_label.bottomAnchor constant:4],
        [_slider.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
        [_slider.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
        [_slider.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-8],
    ]];
}
- (void)didSlide:(UISlider*)s {
    self.value = s.value;
    _valueLabel.text = [NSString stringWithFormat:@"%.0f", s.value];
    if (self.onChange) self.onChange(s.value);
}
@end

// ─────────────────────────────────────────────────────────
#pragma mark - Segmented Cell (bone target)
// ─────────────────────────────────────────────────────────

@interface MESegmentCell : UITableViewCell
@property (nonatomic, copy) NSString* label;
@property (nonatomic) NSInteger selectedIndex;
@property (nonatomic, copy) NSArray<NSString*>* items;
@property (nonatomic, copy) void (^onChange)(NSInteger);
- (void)setup;
@end

@implementation MESegmentCell {
    UILabel*              _label;
    UISegmentedControl*   _seg;
}
- (void)setup {
    self.backgroundColor = [UIColor clearColor];
    self.selectionStyle  = UITableViewCellSelectionStyleNone;

    _label = [UILabel new];
    _label.translatesAutoresizingMaskIntoConstraints = NO;
    _label.textColor = [UIColor whiteColor];
    _label.font      = [UIFont systemFontOfSize:13 weight:UIFontWeightRegular];
    _label.text      = self.label;
    [self.contentView addSubview:_label];

    _seg = [[UISegmentedControl alloc] initWithItems:self.items];
    _seg.translatesAutoresizingMaskIntoConstraints = NO;
    _seg.selectedSegmentIndex = self.selectedIndex;
    [_seg setTitleTextAttributes:@{NSForegroundColorAttributeName:[UIColor whiteColor]} forState:UIControlStateNormal];
    [_seg setTitleTextAttributes:@{NSForegroundColorAttributeName:[UIColor blackColor]} forState:UIControlStateSelected];
    _seg.selectedSegmentTintColor = [UIColor colorWithRed:0.2 green:0.6 blue:1.0 alpha:1.0];
    [_seg addTarget:self action:@selector(didChange:) forControlEvents:UIControlEventValueChanged];
    [self.contentView addSubview:_seg];

    [NSLayoutConstraint activateConstraints:@[
        [_label.topAnchor constraintEqualToAnchor:self.contentView.topAnchor constant:8],
        [_label.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],

        [_seg.topAnchor constraintEqualToAnchor:_label.bottomAnchor constant:6],
        [_seg.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
        [_seg.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
        [_seg.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-8],
    ]];
}
- (void)didChange:(UISegmentedControl*)s {
    self.selectedIndex = s.selectedSegmentIndex;
    if (self.onChange) self.onChange(s.selectedSegmentIndex);
}
@end

// ─────────────────────────────────────────────────────────
#pragma mark - Menu Section Model
// ─────────────────────────────────────────────────────────

@interface MEMenuItem : NSObject
@property (nonatomic, assign) NSInteger type; // 0=toggle 1=slider 2=segment
@property (nonatomic, copy)   NSString* label;
@property (nonatomic, copy)   id        value;
@property (nonatomic, copy)   id        config; // extra: min/max/items
@property (nonatomic, copy)   void (^action)(id);
@end
@implementation MEMenuItem @end

@interface MEMenuSection : NSObject
@property (nonatomic, copy) NSString*           header;
@property (nonatomic, copy) NSArray<MEMenuItem*>* items;
@end
@implementation MEMenuSection @end

// ─────────────────────────────────────────────────────────
#pragma mark - Menu View Controller
// ─────────────────────────────────────────────────────────

@interface MEMenuViewController : UIViewController <UITableViewDataSource, UITableViewDelegate>
@end

@implementation MEMenuViewController {
    UITableView*            _table;
    UIVisualEffectView*     _blur;
    UIView*                 _container;
    UIButton*               _closeBtn;
    NSArray<MEMenuSection*>* _sections;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self buildSections];
    [self setupUI];
}

// ── Build menu data model ─────────────────────────────────
- (void)buildSections {
    // ── AIMBOT SECTION ────────────────────────────────────
    MEMenuItem* aimEnabled = [MEMenuItem new];
    aimEnabled.type  = 0; aimEnabled.label = @"Aimbot";
    aimEnabled.value = @(g_AimbotCfg.enabled);
    aimEnabled.action = ^(id v){ g_AimbotCfg.enabled = [v boolValue]; };

    MEMenuItem* aimSilent = [MEMenuItem new];
    aimSilent.type  = 0; aimSilent.label = @"Silent Aim";
    aimSilent.value = @(g_AimbotCfg.silentAim);
    aimSilent.action = ^(id v){
        g_AimbotCfg.silentAim = [v boolValue];
        if (g_AimbotCfg.silentAim) g_AimbotCfg.legitAim = false;
    };

    MEMenuItem* aimLegit = [MEMenuItem new];
    aimLegit.type  = 0; aimLegit.label = @"Legit Aim";
    aimLegit.value = @(g_AimbotCfg.legitAim);
    aimLegit.action = ^(id v){
        g_AimbotCfg.legitAim = [v boolValue];
        if (g_AimbotCfg.legitAim) g_AimbotCfg.silentAim = false;
    };

    MEMenuItem* aimFOV = [MEMenuItem new];
    aimFOV.type   = 1; aimFOV.label = @"FOV";
    aimFOV.value  = @(g_AimbotCfg.fov);
    aimFOV.config = @{@"min":@0, @"max":@360};
    aimFOV.action = ^(id v){ g_AimbotCfg.fov = [v floatValue]; };

    MEMenuItem* aimSmooth = [MEMenuItem new];
    aimSmooth.type   = 1; aimSmooth.label = @"Smooth";
    aimSmooth.value  = @(g_AimbotCfg.smoothing);
    aimSmooth.config = @{@"min":@1, @"max":@30};
    aimSmooth.action = ^(id v){ g_AimbotCfg.smoothing = [v floatValue]; };

    MEMenuItem* aimBone = [MEMenuItem new];
    aimBone.type   = 2; aimBone.label = @"Bone Target";
    aimBone.value  = @((NSInteger)g_AimbotCfg.boneTarget);
    aimBone.config = @[@"Head", @"Neck", @"Body", @"Pelvis", @"L.Leg", @"R.Leg"];
    aimBone.action = ^(id v){
        g_AimbotCfg.boneTarget = (BoneTarget)[v integerValue];
    };

    MEMenuSection* aimSec = [MEMenuSection new];
    aimSec.header = @"  AIMBOT";
    aimSec.items  = @[aimEnabled, aimSilent, aimLegit, aimFOV, aimSmooth, aimBone];

    // ── ESP SECTION ───────────────────────────────────────
    MEMenuItem* espEnabled = [MEMenuItem new];
    espEnabled.type  = 0; espEnabled.label = @"ESP";
    espEnabled.value = @(g_ESPCfg.enabled);
    espEnabled.action = ^(id v){ g_ESPCfg.enabled = [v boolValue]; };

    MEMenuItem* espBox = [MEMenuItem new];
    espBox.type  = 0; espBox.label = @"Box ESP";
    espBox.value = @(g_ESPCfg.showBox);
    espBox.action = ^(id v){ g_ESPCfg.showBox = [v boolValue]; };

    MEMenuItem* espSkel = [MEMenuItem new];
    espSkel.type  = 0; espSkel.label = @"Skeleton";
    espSkel.value = @(g_ESPCfg.showSkeleton);
    espSkel.action = ^(id v){ g_ESPCfg.showSkeleton = [v boolValue]; };

    MEMenuItem* espHP = [MEMenuItem new];
    espHP.type  = 0; espHP.label = @"HP Bar";
    espHP.value = @(g_ESPCfg.showHP);
    espHP.action = ^(id v){ g_ESPCfg.showHP = [v boolValue]; };

    MEMenuItem* espName = [MEMenuItem new];
    espName.type  = 0; espName.label = @"Name";
    espName.value = @(g_ESPCfg.showName);
    espName.action = ^(id v){ g_ESPCfg.showName = [v boolValue]; };

    MEMenuItem* espDist = [MEMenuItem new];
    espDist.type  = 0; espDist.label = @"Distance";
    espDist.value = @(g_ESPCfg.showDistance);
    espDist.action = ^(id v){ g_ESPCfg.showDistance = [v boolValue]; };

    MEMenuSection* espSec = [MEMenuSection new];
    espSec.header = @"  ESP";
    espSec.items  = @[espEnabled, espBox, espSkel, espHP, espName, espDist];

    // ── MISC SECTION ──────────────────────────────────────
    MEMenuItem* streamProof = [MEMenuItem new];
    streamProof.type  = 0; streamProof.label = @"Streamproof";
    streamProof.value = @(g_StreamProof);
    streamProof.action = ^(id v){
        g_StreamProof = [v boolValue];
        // Toggle overlay window security level
        extern void ME_SetStreamProof(bool);
        ME_SetStreamProof(g_StreamProof);
    };

    MEMenuSection* miscSec = [MEMenuSection new];
    miscSec.header = @"  MISC";
    miscSec.items  = @[streamProof];

    _sections = @[aimSec, espSec, miscSec];
}

// ── UI Setup ─────────────────────────────────────────────
- (void)setupUI {
    self.view.backgroundColor = [UIColor clearColor];

    // Blurry background container
    _container = [[UIView alloc] init];
    _container.translatesAutoresizingMaskIntoConstraints = NO;
    _container.layer.cornerRadius = 16;
    _container.layer.masksToBounds = YES;
    _container.layer.borderWidth = 0.5;
    _container.layer.borderColor = [UIColor colorWithWhite:1.0 alpha:0.2].CGColor;
    [self.view addSubview:_container];

    // Blur effect
    UIBlurEffect* blur = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterialDark];
    _blur = [[UIVisualEffectView alloc] initWithEffect:blur];
    _blur.translatesAutoresizingMaskIntoConstraints = NO;
    [_container addSubview:_blur];

    // Header
    UIView* header = [[UIView alloc] init];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.backgroundColor = [UIColor colorWithRed:0.1 green:0.4 blue:0.9 alpha:0.8];
    [_container addSubview:header];

    UILabel* title = [UILabel new];
    title.translatesAutoresizingMaskIntoConstraints = NO;
    title.text      = @"⚡ ModEngine";
    title.textColor = [UIColor whiteColor];
    title.font      = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
    [header addSubview:title];

    // Close button
    _closeBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    _closeBtn.translatesAutoresizingMaskIntoConstraints = NO;
    [_closeBtn setTitle:@"✕" forState:UIControlStateNormal];
    _closeBtn.tintColor = [UIColor whiteColor];
    _closeBtn.titleLabel.font = [UIFont systemFontOfSize:16 weight:UIFontWeightBold];
    [_closeBtn addTarget:self action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
    [header addSubview:_closeBtn];

    // Table
    _table = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStyleGrouped];
    _table.translatesAutoresizingMaskIntoConstraints = NO;
    _table.dataSource = self;
    _table.delegate   = self;
    _table.backgroundColor = [UIColor clearColor];
    _table.separatorColor  = [UIColor colorWithWhite:1.0 alpha:0.1];
    [_container addSubview:_table];

    // Layout
    CGFloat menuW = MIN(310, UIScreen.mainScreen.bounds.size.width - 40);
    CGFloat menuH = MIN(520, UIScreen.mainScreen.bounds.size.height - 100);

    [NSLayoutConstraint activateConstraints:@[
        [_container.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [_container.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [_container.widthAnchor  constraintEqualToConstant:menuW],
        [_container.heightAnchor constraintEqualToConstant:menuH],

        [_blur.topAnchor    constraintEqualToAnchor:_container.topAnchor],
        [_blur.bottomAnchor constraintEqualToAnchor:_container.bottomAnchor],
        [_blur.leadingAnchor constraintEqualToAnchor:_container.leadingAnchor],
        [_blur.trailingAnchor constraintEqualToAnchor:_container.trailingAnchor],

        [header.topAnchor    constraintEqualToAnchor:_container.topAnchor],
        [header.leadingAnchor constraintEqualToAnchor:_container.leadingAnchor],
        [header.trailingAnchor constraintEqualToAnchor:_container.trailingAnchor],
        [header.heightAnchor  constraintEqualToConstant:44],

        [title.centerYAnchor constraintEqualToAnchor:header.centerYAnchor],
        [title.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:16],

        [_closeBtn.centerYAnchor constraintEqualToAnchor:header.centerYAnchor],
        [_closeBtn.trailingAnchor constraintEqualToAnchor:header.trailingAnchor constant:-8],
        [_closeBtn.widthAnchor  constraintEqualToConstant:40],
        [_closeBtn.heightAnchor constraintEqualToConstant:40],

        [_table.topAnchor    constraintEqualToAnchor:header.bottomAnchor],
        [_table.bottomAnchor constraintEqualToAnchor:_container.bottomAnchor],
        [_table.leadingAnchor constraintEqualToAnchor:_container.leadingAnchor],
        [_table.trailingAnchor constraintEqualToAnchor:_container.trailingAnchor],
    ]];

    // Drag gesture
    UIPanGestureRecognizer* pan = [[UIPanGestureRecognizer alloc]
        initWithTarget:self action:@selector(handlePan:)];
    [header addGestureRecognizer:pan];

    // Entrance animation
    _container.alpha = 0;
    _container.transform = CGAffineTransformMakeScale(0.85, 0.85);
    [UIView animateWithDuration:0.25 delay:0
        usingSpringWithDamping:0.7 initialSpringVelocity:0.5
        options:0 animations:^{
            self->_container.alpha = 1;
            self->_container.transform = CGAffineTransformIdentity;
        } completion:nil];
}

- (void)handlePan:(UIPanGestureRecognizer*)gr {
    CGPoint t = [gr translationInView:self.view];
    _container.center = CGPointMake(_container.center.x + t.x, _container.center.y + t.y);
    [gr setTranslation:CGPointZero inView:self.view];
}

- (void)close {
    [UIView animateWithDuration:0.2 animations:^{
        self->_container.alpha = 0;
        self->_container.transform = CGAffineTransformMakeScale(0.85, 0.85);
    } completion:^(BOOL f){
        [self.view removeFromSuperview];
        [self removeFromParentViewController];
    }];
}

// ── UITableViewDataSource ─────────────────────────────────
- (NSInteger)numberOfSectionsInTableView:(UITableView*)tv {
    return (NSInteger)_sections.count;
}
- (NSInteger)tableView:(UITableView*)tv numberOfRowsInSection:(NSInteger)sec {
    return (NSInteger)_sections[sec].items.count;
}
- (NSString*)tableView:(UITableView*)tv titleForHeaderInSection:(NSInteger)sec {
    return _sections[sec].header;
}
- (void)tableView:(UITableView*)tv willDisplayHeaderView:(UIView*)view forSection:(NSInteger)sec {
    if ([view isKindOfClass:[UITableViewHeaderFooterView class]]) {
        UITableViewHeaderFooterView* hv = (UITableViewHeaderFooterView*)view;
        hv.textLabel.textColor = [UIColor colorWithRed:0.4 green:0.7 blue:1.0 alpha:1.0];
        hv.textLabel.font = [UIFont systemFontOfSize:11 weight:UIFontWeightSemibold];
    }
}
- (UITableViewCell*)tableView:(UITableView*)tv cellForRowAtIndexPath:(NSIndexPath*)ip {
    MEMenuItem* item = _sections[ip.section].items[ip.row];
    switch (item.type) {
        case 0: { // Toggle
            METoggleCell* c = [METoggleCell new];
            c.label    = item.label;
            c.value    = [item.value boolValue];
            c.onToggle = ^(BOOL v){ item.value = @(v); item.action(@(v)); };
            [c setup];
            return c;
        }
        case 1: { // Slider
            MESliderCell* c = [MESliderCell new];
            c.label  = item.label;
            c.value  = [item.value floatValue];
            c.minVal = [item.config[@"min"] floatValue];
            c.maxVal = [item.config[@"max"] floatValue];
            c.onChange = ^(float v){ item.value = @(v); item.action(@(v)); };
            [c setup];
            return c;
        }
        case 2: { // Segment
            MESegmentCell* c = [MESegmentCell new];
            c.label         = item.label;
            c.selectedIndex = [item.value integerValue];
            c.items         = item.config;
            c.onChange      = ^(NSInteger i){ item.value = @(i); item.action(@(i)); };
            [c setup];
            return c;
        }
    }
    return [UITableViewCell new];
}
- (CGFloat)tableView:(UITableView*)tv heightForRowAtIndexPath:(NSIndexPath*)ip {
    MEMenuItem* item = _sections[ip.section].items[ip.row];
    return item.type == 0 ? 44.0 : 72.0;
}

@end

// ─────────────────────────────────────────────────────────
#pragma mark - Overlay Window
// ─────────────────────────────────────────────────────────

// UITextEffectsWindow subclass — invisible to screen recording
@interface MEOverlayWindow : UIWindow
@end
@implementation MEOverlayWindow
// Prevent capture (Fl0rk ScreenProtectorKit-Spoof approach)
- (BOOL)_shouldCreateContextForDisplay { return NO; }
- (UIView*)snapshotViewAfterScreenUpdates:(BOOL)b { return nil; }
@end

static MEOverlayWindow*        g_OverlayWindow  = nil;
static MEMenuViewController*   g_MenuVC         = nil;
static bool                    g_MenuVisible     = false;

// Called from triple-tap recognizer
void ME_ShowMenu() {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_MenuVisible) return;
        g_MenuVisible = true;

        if (!g_OverlayWindow) {
            g_OverlayWindow = [[MEOverlayWindow alloc]
                initWithWindowScene:UIApplication.sharedApplication.connectedScenes.allObjects.firstObject];
            g_OverlayWindow.windowLevel = UIWindowLevelAlert + 100;
            g_OverlayWindow.backgroundColor = [UIColor clearColor];
            g_OverlayWindow.hidden = NO;
            g_OverlayWindow.rootViewController = [UIViewController new];
        }

        g_MenuVC = [MEMenuViewController new];
        g_MenuVC.view.frame = g_OverlayWindow.bounds;
        [g_OverlayWindow.rootViewController addChildViewController:g_MenuVC];
        [g_OverlayWindow.rootViewController.view addSubview:g_MenuVC.view];
        [g_MenuVC didMoveToParentViewController:g_OverlayWindow.rootViewController];
    });
}

void ME_SetStreamProof(bool on) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_OverlayWindow) {
            // When streamproof ON: use _shouldCreateContextForDisplay=NO
            // When OFF: normal window
            g_OverlayWindow.hidden = YES;
            g_OverlayWindow.hidden = NO;
        }
    });
}

// ── Triple-tap gesture setup ──────────────────────────────
void ME_SetupGesture() {
    dispatch_async(dispatch_get_main_queue(), ^{
        UITapGestureRecognizer* tap = [[UITapGestureRecognizer alloc]
            initWithTarget:nil action:nil];
        tap.numberOfTapsRequired    = 3;
        tap.numberOfTouchesRequired = 1;

        [tap addTarget:[[NSObject alloc] init] action:nil];
        // Use block-based approach via UIAction (iOS 14+)
        // Fallback: observe touches on window level

        // Simple approach: observe UIApplicationDidBecomeActiveNotification
        // then add tap to key window
        [[NSNotificationCenter defaultCenter]
            addObserverForName:UIWindowDidBecomeKeyNotification
            object:nil queue:[NSOperationQueue mainQueue]
            usingBlock:^(NSNotification* n) {
                UIWindow* win = n.object;
                if (!win || win == g_OverlayWindow) return;
                UITapGestureRecognizer* t = [[UITapGestureRecognizer alloc]
                    initWithTarget:nil action:nil];
                t.numberOfTapsRequired    = 3;
                t.numberOfTouchesRequired = 1;
                // Use associated object to hold the block
                __block id obs = [[NSNotificationCenter defaultCenter] // capture
                    addObserverForName:@"__ME_TAP__" object:nil queue:nil usingBlock:nil];
                [t addTarget:[NSBlockOperation blockOperationWithBlock:^{ ME_ShowMenu(); }]
                    action:@selector(main)];
                [win addGestureRecognizer:t];
            }];
    });
}
