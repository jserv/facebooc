describe("", function() {
  var rootEl;
  beforeEach(function() {
    rootEl = browser.rootEl;
    browser.get("build/docs/examples/example-ngMessageFormat-example/index-jquery.html");
  });
  
describe('MessageFormat plural', function() {
  it('should pluralize initial values', function() {
    var messageElem = element(by.binding('recipients.length')), decreaseRecipientsBtn = element(by.id('decreaseRecipients'));
    expect(messageElem.getText()).toEqual('Harry Potter gave Alice and 2 other people a gift (#=2)');
    decreaseRecipientsBtn.click();
    expect(messageElem.getText()).toEqual('Harry Potter gave Alice and one other person a gift (#=1)');
    decreaseRecipientsBtn.click();
    expect(messageElem.getText()).toEqual('Harry Potter gave one gift to Alice (#=0)');
    decreaseRecipientsBtn.click();
    expect(messageElem.getText()).toEqual('Harry Potter gave no gifts (#=-1)');
  });
});
});