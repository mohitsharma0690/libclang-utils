#import <Foundation/Foundation.h>

@interface Hello:NSObject
- (void)helloWorld;
@end

@implementation Hello 
- (void)helloWorld {
    NSLog(@"Hello World!");
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(unusedFunction) 
                                                 name:@"sampleNotification" object:nil];
}

- (void)unusedFunction {
    NSLog(@"This is an unused function");
}
@end

@interface GoodMorning:NSObject
- (void)goodMorning;
@end
@implementation GoodMorning
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
- (void)goodMorning {
    NSLog(@"Good Morning!");
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(unusedFunction) 
                                                 name:@"sampleNotification" object:nil];
}
- (void)unusedFunction {
}
@end

int main() {
    Hello *h = [[Hello alloc] init];
    [h helloWorld];
    GoodMorning *m= [[GoodMorning alloc] init];
    [m goodMorning];
    return 0;
}
