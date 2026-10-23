(function (global) {
  function signatureOf(value) {
    if (typeof value === "boolean") {
      return value ? "true" : "false";
    }
    if (value !== null && typeof value === "object") {
      return JSON.stringify(value);
    }
    return String(value);
  }

  function DraftStore() {
    this._drafts = {};
  }

  DraftStore.prototype.beginEdit = function (fieldId, pendingValue) {
    this._drafts[fieldId] = {
      pendingValue: pendingValue,
      pendingSignature: signatureOf(pendingValue),
    };
  };

  DraftStore.prototype.isDirty = function (fieldId) {
    return Object.prototype.hasOwnProperty.call(this._drafts, fieldId);
  };

  DraftStore.prototype.overlay = function (fieldId, serverValue) {
    const draft = this._drafts[fieldId];
    return draft ? draft.pendingValue : serverValue;
  };

  DraftStore.prototype.confirm = function (fieldId, serverValue) {
    const draft = this._drafts[fieldId];
    if (!draft) {
      return false;
    }
    if (draft.pendingSignature !== signatureOf(serverValue)) {
      return false;
    }
    delete this._drafts[fieldId];
    return true;
  };

  DraftStore.prototype.revert = function (fieldId) {
    delete this._drafts[fieldId];
  };

  global.WattcycleDraftStore = DraftStore;
})(window);
